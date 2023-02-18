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
#ifndef _MSWMM_PROJECT_HPP
#define _MSWMM_PROJECT_HPP

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdexcept>

#include <qdom.h>

#include "compoundfilereader.h"
#include "utf.h"
#include "TimelineItem.hpp"


namespace mswmm {

struct CorruptFileError : public std::runtime_error {
  CorruptFileError(std::string desc) : std::runtime_error(desc) {}
};



enum class TrackType {
  VIDEO = 0,
  AUDIO = 1,
  SOMETHING = 5 // This is probably the title overlay timeline, but not sure
};



class Project {
  public:
    Project(std::string path);
    ~Project();
    void printXml(std::ostream& target, uint8_t indent = 2) const;
    void printMetadata(std::ostream& target, uint8_t indent = 0) const;
    void printFiles(std::ostream& target, uint8_t indent = 0) const;
    void printMediaTimeline(std::ostream& target, TrackType trackId, uint8_t indent = 0) const;
    std::string generateFfmpegCommand(std::vector<std::pair<std::string, std::string>> substitutions) const;

    bool hasTitleSequences;
    size aspectRatio;
    std::string author;
    std::string title;
    std::string description;
    std::string copyright;
    std::string rating;
    std::vector<std::string> sourceFiles;
    std::vector<mswmm::TimelineItem*> videoTimeline;
    std::vector<mswmm::TimelineItem*> audioTimeline;
  private:
    void analyzeXml();
    void getMetadata(QDomElement const& dataStr);
    void getFileList(QDomElement const& dataStr);
    void getMediaTimeline(QDomElement const& dataStr, TrackType trackId);
    QDomElement getTagWithAttr(QDomNode const& parent, QString tag, QString attr, QString attrVal) const;
    CFB::COMPOUND_FILE_ENTRY const* findStream(CFB::CompoundFileReader const& reader, char const* targetName) const;

    QDomDocument xmlDoc;
};

} // Namespace mswmm

#endif
