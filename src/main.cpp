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

See LICENCE file for the full license text.
*******************************************************************/
#include <memory>
#include <fstream>
#include <iostream>

#include <QString>
#include <QDomDocument>

#include "utf.h"
#include "compoundfilereader.h"
#include "TimelineItem.hpp"



CFB::COMPOUND_FILE_ENTRY const* findStream(CFB::CompoundFileReader const& reader, char const* streamName) {
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


/**
 * @brief Get an XML DOM node that has a specific attribute.
 *
 * @param parent The parent to search.
 * @param tag The name of the tag searched.
 * @param attr The name of the attribute that tag should carry.
 * @param attrVal The value the attribute must have.
 * @return QDomElement The first DOM element found; might be Null.
 */
QDomElement getTagWithAttr(QDomNode const& parent, QString tag, QString attr, QString attrVal) {
  QDomElement n = parent.firstChildElement(tag);
  while (!n.isNull()) {
    if (n.hasAttribute(attr) && n.attribute(attr) == attrVal) {
      break;
    }
    n = n.nextSiblingElement(tag);
  }
  return n;
}



int main() {
  // Open file.
  std::string filename = "example-projects/6.0.6000.16386/empty-with-metadata.MSWMM";
  std::ifstream file(filename, std::ios_base::in | std::ios_base::binary);
  if (!file.good()) {
    std::cout << "Can't open file." << std::endl;
    return 1;
  }

  // Get file size.
  file.seekg(0, file.end);
  size_t length = file.tellg();
  file.seekg(0, file.beg);

  // Read file into buffer.
  auto buffer = std::make_unique<char[]>(length);
  file.read(buffer.get(), length);
  // Create XML DOM.
  QDomDocument xmlDoc("mswmm-project");
  try {
    // Parse CFB file.
    CFB::CompoundFileReader reader(buffer.get(), length);

    // Get XML file defining the MSWMM project.
    auto xmlStream = findStream(reader, "ProducerData\\Producer.Dat");
    if (!xmlStream) {
      std::cout << "Can't find project data in file." << std::endl;
      return 1;
    }
    // The following code assumes that the buffer contains UTF-16 text.
    if (xmlStream->size % 2 != 0) {
      std::cout << "Project XML is unexpectedly not encoded as UTF-16. Parsing stopped." << std::endl;
      return 1;
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
      std::cout << "Can't parse project definition XML." << std::endl;
      std::cout << errorStr.toStdString() << std::endl;
      std::cout << "Pos: " << errorLine << "|" << errorCol << std::endl;
      return 1;
    }
  }
  catch (CFB::WrongFormat& e) {
    std::cout << e.what() << std::endl;
    return 1;
  }


  // Extract data from XML DOM.
  auto xmlRoot = xmlDoc.documentElement();
  auto dataStr = xmlRoot.firstChildElement("Project")
                        .firstChildElement("DataStr");
  auto producerProperties = dataStr.firstChildElement("ProducerProperties");

  std::cout << "Metadata:\n";
  QDomElement n = producerProperties.firstChildElement("MetDat");
  while (!n.isNull()) {
    auto attr = n.attributes();
    std::string key   = attr.namedItem("MDTag").nodeValue().toStdString();
    std::string value = attr.namedItem("MDVal").nodeValue().toStdString();
    std::cout << "  " << key << ": " << value << "\n";
    n = n.nextSiblingElement("MetDat");
  }

  std::cout << "Files used in project:\n";
  n = dataStr.firstChildElement("FileInfo");
  while (!n.isNull()) {
    auto fileInfo = n.attributes();
    std::string src = fileInfo.namedItem("SrceFn").nodeValue().toStdString();
    std::cout << "  " << src << "\n";
    n = n.nextSiblingElement("FileInfo");
  }


  // Get video track.
  std::cout << "Video timeline:\n";
  n = getTagWithAttr(dataStr, "Track", "TrackTyp", "0");
  if (n.isNull()) {
    std::cout << "Can't find video track!" << std::endl;
    return 1;
  }

  // Get array of video clips.
  QString videoArrUid = n.firstChildElement("TrkClips").attribute("UID");
  QDomElement videoArr = getTagWithAttr(dataStr, "TIArr", "UID", videoArrUid);
  n = videoArr.firstChildElement("UID");
  while (!n.isNull()) {
    mswmm::TimelineItem ti;
    QString tmlnItemUid = n.attribute("UID");
    QDomElement tmlnItem = getTagWithAttr(dataStr, "", "UID", tmlnItemUid);
    ti.timelineStart = tmlnItem.attribute("TmlnSrt").toFloat();
    ti.timelineEnd = tmlnItem.attribute("TmlnEnd").toFloat();
    ti.sourceStart = tmlnItem.attribute("ClpSrt").toFloat();
    ti.sourceEnd = tmlnItem.attribute("ClpEnd").toFloat();

    if (tmlnItem.tagName() == "TiTitleSource") {
      std::cout << "Title from " << ti.timelineStart << "s to " << ti.timelineEnd << "s\n";
      n = n.nextSiblingElement("UID");
      continue;
    }

    QString clipItemUid = tmlnItem.firstChildElement("ClipWMItem").attribute("UID");
    QDomElement clipItem = getTagWithAttr(dataStr, "", "UID", clipItemUid);
    QString avSourceUid = clipItem.firstChildElement("Srce").attribute("UID");
    QDomElement avSource = getTagWithAttr(dataStr, "AVSource", "UID", avSourceUid);
    QString fileInfoUid = avSource.attribute("FileID");
    QDomElement fileInfo = getTagWithAttr(dataStr, "FileInfo", "FileID", fileInfoUid);

    ti.name = clipItem.attribute("ClpNam").toStdString();
    ti.srcPath = fileInfo.attribute("SrceFn").toStdString();
    ti.fileSizeKiBi = avSource.attribute("FileSize").toLongLong();
    ti.srcSizePx.x = avSource.attribute("SrcWidth").toLongLong();
    ti.srcSizePx.y = avSource.attribute("SrcHeight").toLongLong();


    std::cout << ti.name << " from " << ti.timelineStart << "s to " << ti.timelineEnd << "s\n"
              << "  - Path: " << ti.srcPath << "\n"
              << "  - Part taken from file: " << ti.sourceStart << "s to " << ti.sourceEnd << "s\n"
              << "  - File size: ca. " << ti.fileSizeKiBi << "kiB\n"
              << "  - Width x Height: " << ti.srcSizePx.x << "px x " << ti.srcSizePx.y << "px\n";
    n = n.nextSiblingElement("UID");
  }


  return 0;
}
