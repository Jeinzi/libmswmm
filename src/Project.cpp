/*******************************************************************
libmswmm: Read Microsoft Windows Movie Maker (.mswmm) files.
Copyright © 2023 Julian Heinzel

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

See LICENSE file for the full license text.
*******************************************************************/
#include "Project.hpp"


namespace mswmm {

Project::Project(std::string path) {
  hasTitleSequences = false;

  std::ifstream file(path, std::ios_base::in | std::ios_base::binary);
  if (!file.good()) {
    throw std::runtime_error("Can't open file '" + path + "'.");
  }

  // Get file size.
  file.seekg(0, file.end);
  size_t length = file.tellg();
  file.seekg(0, file.beg);

  // Read file into buffer.
  auto buffer = std::make_unique<char[]>(length);
  file.read(buffer.get(), length);
  // Create XML DOM.
  try {
    // Parse CFB file.
    CFB::CompoundFileReader reader(buffer.get(), length);

    // Get XML file defining the MSWMM project.
    auto xmlStream = findStream(reader, "ProducerData\\Producer.Dat");
    if (!xmlStream) {
      throw mswmm::CorruptFileError("Can't find project definition XML (Producer.Dat).");
    }
    // The following code assumes that the buffer contains UTF-16 text.
    if (xmlStream->size % 2 != 0) {
      throw mswmm::CorruptFileError("Project XML is not encoded as UTF-16.");
    }
    // Read XML into buffer.
    auto xmlBuffer = std::make_unique<char[]>(xmlStream->size+2);
    reader.ReadFile(xmlStream, 0, xmlBuffer.get(), xmlStream->size);
    // Add a zero termination for good measure, so
    // QString::fromUtf16() definitely terminates.
    xmlBuffer[xmlStream->size] = 0;
    xmlBuffer[xmlStream->size+1] = 0;
    auto xmlString = QString::fromUtf16((char16_t*)xmlBuffer.get()).trimmed();

    // Parse XML.
    QString errorStr;
    int errorLine;
    int errorCol;
    if (!xmlDoc.setContent(xmlString, false, &errorStr, &errorLine, &errorCol)) {
      std::string error = "Can't parse project XML (Producer.Dat) at line " +
                          std::to_string(errorLine) +
                          ", column " +
                          std::to_string(errorCol) +
                          ": " +
                          errorStr.toStdString();
      throw mswmm::CorruptFileError(error);
    }
  }
  catch (CFB::WrongFormat& e) {
    throw mswmm::CorruptFileError("Can't parse CFB container: " + std::string(e.what()));
  }

  analyzeXml();
}



Project::~Project() {
  for (auto& ti: videoTimeline) {
    delete ti;
  }
  for (auto& ti: audioTimeline) {
    delete ti;
  }
}



void Project::printXml(std::ostream& target, uint8_t indent) const {
  target << xmlDoc.toString(indent).toStdString();
}



void Project::printMetadata(std::ostream& target, uint8_t indent) const {
  target << std::string(indent, ' ') << "Aspect ratio: " << aspectRatio.x << ":" << aspectRatio.y << '\n';
  target << std::string(indent, ' ') << "Author: " << author << '\n';
  target << std::string(indent, ' ') << "Title: " << title << '\n';
  target << std::string(indent, ' ') << "Description: " << description << '\n';
  target << std::string(indent, ' ') << "Copyright: " << copyright << '\n';
  target << std::string(indent, ' ') << "Rating: " << rating << '\n';
}



void Project::printFiles(std::ostream& target, uint8_t indent) const {
  for (auto const& f: sourceFiles) {
    target << std::string(indent, ' ') << f << '\n';
  }
}



void Project::printMediaTimeline(std::ostream& target, TrackType trackId, uint8_t indent) const {
  std::vector<TimelineItem*> const* timeline;
  switch (trackId) {
    case TrackType::VIDEO:
      timeline = &videoTimeline;
      break;
    case TrackType::AUDIO:
      timeline = &audioTimeline;
      break;
    default:
      return;
  }

  for (auto const& ti: *timeline) {
    ti->printItem(target, indent);
  }
}


/**
 * @brief Generate an ffmpeg command to render the video described
 * in the Movie Maker project, if possible. Throws exceptions if not.
 * Only the video timeline is used for now.
 *
 * @param substitutions A list of string substitutions that will be performed on the source file paths.
 * All occurrences of pair.first will be replaced with pair.second.
 * @return std::string The ffmpeg command.
 */
std::string Project::generateFfmpegCommand(std::vector<std::pair<std::string, std::string>> substitutions) const {
  if (videoTimeline.size() == 0) {
    throw std::runtime_error("Empty video timeline.");
  }
  std::stringstream command;
  std::stringstream filter;
  command << "ffmpeg ";

  size lastSizePx;
  bool hasImages = false;
  bool hasVideos = false;
  unsigned int i = 0;
  float lastEndTime = 0;
  for (auto const& ti: videoTimeline) {
    if (ti->timelineStart < lastEndTime) {
      throw std::runtime_error("Timeline items overlap, but transitions are not yet supported.");
    }

    // Images should be easy to include, title sequences not at all.
    TimelineVideoItem* tvi = dynamic_cast<TimelineVideoItem*>(ti);
    TimelineStillItem* tsi = dynamic_cast<TimelineStillItem*>(ti);
    std::string path;
    size currentSizePx;
    if (tvi) {
      hasVideos = true;
      path = tvi->srcPath;
      currentSizePx = tvi->srcSizePx;
    }
    else if (tsi) {
      hasImages = true;
      path = tsi->srcPath;
      currentSizePx = tsi->srcSizePx;
    }
    else {
      throw std::runtime_error("Only videos and images are currently supported on the timeline.");
    }

    if (hasVideos && hasImages) {
      throw std::runtime_error("Timelines with both videos and images are not yet supported.");
    }

    if (i != 0 && currentSizePx != lastSizePx) {
      throw std::runtime_error("Images don't have the same size.");
    }

    // Replace substrings if requested.
    for (auto const& sp: substitutions) {
      while (true) {
        size_t startPos = path.find(sp.first);
        if (startPos == std::string::npos) {
          break;
        }
        else {
          path.replace(startPos, sp.first.length(), sp.second);
        }
      }
    }

    // Assemble command.
    if (tvi) {
      command << "-ss " << tvi->sourceStart
              << " -to " << tvi->sourceEnd
              << " -i '" << path << "' ";
      filter << "[" << i << ":v] [" << i << ":a] ";
    }
    else if (tsi) {
      command << "-loop 1 -framerate 24 "
              << "-t " << ti->timelineEnd - ti->timelineStart
              << " -i '" << path << "' ";
      filter << "[" << i << "] ";
    }


    lastEndTime = ti->timelineEnd;
    lastSizePx = currentSizePx;
    ++i;
  }

  // Add concatenation filter.
  if (hasVideos) {
    command << "-filter_complex '" << filter.str() << "concat=n=" << i << ":v=1:a=1 [v] [a]' -map '[v]' -map '[a]' ";
  }
  else {
    command << "-filter_complex '" << filter.str() << "concat=n=" << i << ":v=1:a=0' ";
  }

  command << "output.mp4";
  return command.str();
}



void Project::analyzeXml() {
  // Extract data from XML DOM.
  auto xmlRoot = xmlDoc.documentElement();
  auto dataStr = xmlRoot.firstChildElement("Project")
                        .firstChildElement("DataStr");
  if (dataStr.isNull()) {
    throw CorruptFileError("Can't find 'DataStr' XML tag, which should contain the entire project definition.");
  }


  getMetadata(dataStr);
  getFileList(dataStr);
  getMediaTimeline(dataStr, TrackType::VIDEO);
  getMediaTimeline(dataStr, TrackType::AUDIO);
}



void Project::getMetadata(QDomElement const& dataStr) {
  QDomElement producerProperties = dataStr.firstChildElement("ProducerProperties");
  aspectRatio.x = producerProperties.attribute("ProjectAspectRatioX").toULong();
  aspectRatio.y = producerProperties.attribute("ProjectAspectRatioY").toULong();

  QDomElement n = producerProperties.firstChildElement("MetDat");
  while (!n.isNull()) {
    auto attr = n.attributes();
    std::string key   = attr.namedItem("MDTag").nodeValue().toStdString();
    std::string value = attr.namedItem("MDVal").nodeValue().toStdString();
    if (key == "Author") {
      author = value;
    }
    else if (key == "PresentationTitle") {
      title = value;
    }
    else if (key == "Copyright") {
      copyright = value;
    }
    else if (key == "Rating") {
      rating = value;
    }
    else if (key == "Description") {
      description = value;
    }
    n = n.nextSiblingElement("MetDat");
  }
}



void Project::getFileList(QDomElement const& dataStr) {
  QDomElement n = dataStr.firstChildElement("FileInfo");
  while (!n.isNull()) {
    auto src = n.attribute("SrceFn").toStdString();
    sourceFiles.emplace_back(std::move(src));
    n = n.nextSiblingElement("FileInfo");
  }
}



void Project::getMediaTimeline(QDomElement const& dataStr, TrackType trackId) {
  std::vector<TimelineItem*>* timeline;
  switch (trackId) {
    case TrackType::VIDEO:
      timeline = &videoTimeline;
      break;
    case TrackType::AUDIO:
      timeline = &audioTimeline;
      break;
    default:
      throw std::runtime_error("Other timelines than video and audio are currently not supported.");
  }

  // Get video or audio track.
  auto trackIdStr = QString::number(static_cast<uint>(trackId));
  QDomElement n = getTagWithAttr(dataStr, "Track", "TrackTyp", trackIdStr);
  if (n.isNull()) {
    std::string trackName = (trackId == TrackType::VIDEO ? "video" : "audio");
    throw CorruptFileError("Can't find " + trackName + " track!");
  }

  // Get array of timeline items.
  // May contain videos, images, audio files and title sequences.
  QString videoArrUid = n.firstChildElement("TrkClips").attribute("UID");
  QDomElement videoArr = getTagWithAttr(dataStr, "TIArr", "UID", videoArrUid);
  n = videoArr.firstChildElement("UID");
  for (; !n.isNull();  n = n.nextSiblingElement("UID")) {
    // Traverse DOM to get all the elements that contain information
    // regarding the current section on the timeline.
    // TiTitleSource doesn't have anything after tmlnItem, so
    // clipItem, avSource and fileInfo will all be null in that
    // case, and all attribute values taken from them will also be
    // as empty as possible.
    QString tmlnItemUid = n.attribute("UID");
    QDomElement tmlnItem = getTagWithAttr(dataStr, "", "UID", tmlnItemUid);
    QString clipItemUid = tmlnItem.firstChildElement("ClipWMItem").attribute("UID");
    QDomElement clipItem = getTagWithAttr(dataStr, "", "UID", clipItemUid);
    QString avSourceUid = clipItem.firstChildElement("Srce").attribute("UID");
    QDomElement avSource = getTagWithAttr(dataStr, "AVSource", "UID", avSourceUid);
    QString fileInfoUid = avSource.attribute("FileID");
    QDomElement fileInfo = getTagWithAttr(dataStr, "FileInfo", "FileID", fileInfoUid);

    // Every timeline item has a start and end time.
    float timelineStart = tmlnItem.attribute("TmlnSrt").toFloat();
    float timelineEnd = tmlnItem.attribute("TmlnEnd").toFloat();

    // Only images, videos and audio files have the following attributes.
    std::string name = clipItem.attribute("ClpNam").toStdString();
    std::string srcPath = fileInfo.attribute("SrceFn").toStdString();
    size_t fileSizeKiB = avSource.attribute("FileSize").toULong();

    // X and Y dimensions are only non-zero for images and videos.
    mswmm::size srcSizePx {
      avSource.attribute("SrcWidth").toULong(),
      avSource.attribute("SrcHeight").toULong()
    };

    // Taking only parts of the input files is only possible
    // for video and audio files.
    float sourceStart = tmlnItem.attribute("ClpSrt").toFloat();
    float sourceEnd = tmlnItem.attribute("ClpEnd").toFloat();


    auto tag = tmlnItem.tagName();
    if (tag == "TiTitleSource") {
      if (trackId == TrackType::AUDIO) {
        throw CorruptFileError("Title sequence in audio timeline.");
      }
      hasTitleSequences = true;
      auto ti = new TimelineTitleItem;
      ti->timelineStart = timelineStart;
      ti->timelineEnd = timelineEnd;
      timeline->push_back(ti);
    }
    else if (tag == "TmlnStillItem") {
      if (trackId == TrackType::AUDIO) {
        throw CorruptFileError("Picture in audio timeline.");
      }
      auto ti = new TimelineStillItem;
      ti->timelineStart = timelineStart;
      ti->timelineEnd = timelineEnd;
      ti->name = name;
      ti->srcPath = srcPath;
      ti->fileSizeKiB = fileSizeKiB;
      ti->srcSizePx = srcSizePx;
      timeline->push_back(ti);
    }
    else if (tag == "TmlnVideoItem" || tag == "TmlnAudioItem") {
      TimelineVideoItem** ti = new TimelineVideoItem*;
      if (tag == "TmlnVideoItem") {
        *ti = new TimelineVideoItem;
      }
      else {
        *ti = new TimelineAudioItem;
      }

      (*ti)->timelineStart = timelineStart;
      (*ti)->timelineEnd = timelineEnd;
      (*ti)->name = name;
      (*ti)->srcPath = srcPath;
      (*ti)->fileSizeKiB = fileSizeKiB;
      (*ti)->srcSizePx = srcSizePx;
      (*ti)->sourceStart = sourceStart;
      (*ti)->sourceEnd = sourceEnd;
      timeline->push_back(*ti);
      delete ti;
    }
  }
}



/**
* @brief Get an XML DOM node that has a specific attribute.
*
* @param parent The parent to search.
* @param tag The name of the tag searched.
* @param attr The name of the attribute that tag should carry.
* @param attrVal The value the attribute must have.
* @return QDomElement The first DOM element found; might be Null.
*/
QDomElement Project::getTagWithAttr(QDomNode const& parent, QString tag, QString attr, QString attrVal) const {
  QDomElement n = parent.firstChildElement(tag);
  while (!n.isNull()) {
    if (n.hasAttribute(attr) && n.attribute(attr) == attrVal) {
      break;
    }
    n = n.nextSiblingElement(tag);
  }
  return n;
}



CFB::COMPOUND_FILE_ENTRY const* Project::findStream(CFB::CompoundFileReader const& reader, char const* streamName) const {
  CFB::COMPOUND_FILE_ENTRY const* ret = nullptr;
  reader.EnumFiles(reader.GetRootEntry(), -1, [&](CFB::COMPOUND_FILE_ENTRY const* entry,
                                                  CFB::utf16string const& u16dir,
                                                  [[maybe_unused]] int level)
                                                  ->void
  {
    if (reader.IsStream(entry)) {
      std::string name = UTF16ToUTF8(entry->name);
      if (u16dir.length() > 0) {
        std::string dir = UTF16ToUTF8(u16dir.c_str());
        bool dirMatches = !strncmp(streamName, dir.c_str(), dir.length());
        bool hasBackslash = streamName[dir.length()] == '\\';
        bool nameMatches = !strcmp(streamName + dir.length() + 1, name.c_str());
        if (dirMatches && hasBackslash && nameMatches) {
          ret = entry;
        }
      }
      else if (strcmp(streamName, name.c_str()) == 0) {
        ret = entry;
      }
    }
  });
  return ret;
}

} // Namespace mswmm
