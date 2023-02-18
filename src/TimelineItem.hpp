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
#ifndef _TIMELINEITEM_HPP
#define _TIMELINEITEM_HPP

#include <string>
#include <ostream>


namespace mswmm {

struct size {
  size_t x;
  size_t y;
};



struct TimelineItem {
  float timelineStart;
  float timelineEnd;

  virtual ~TimelineItem() = default;
  virtual void printItem(std::ostream& stream, uint8_t indent = 0) const = 0;
};



struct TimelineTitleItem : public TimelineItem {
  //std::string type;
  void printItem(std::ostream& stream, uint8_t indent = 0) const override {
    stream << std::string(indent, ' ') << "Title from " << timelineStart << "s to " << timelineEnd << "s\n";
  }
};



struct TimelineStillItem : public TimelineItem {
  std::string srcPath;
  std::string name;
  size_t fileSizeKiB;
  size srcSizePx;

  void printItem(std::ostream& stream, uint8_t indent = 0) const override {
    std::string indentStr = std::string(indent, ' ');
    stream << indentStr << "'" << name << "' from " << timelineStart << "s to " << timelineEnd << "s\n"
           << indentStr << "  - Path: " << srcPath << "\n"
           << indentStr << "  - File size: ca. " << fileSizeKiB << "KiB\n"
           << indentStr << "  - Width x Height: " << srcSizePx.x << "px x " << srcSizePx.y << "px\n";
  }
};



struct TimelineVideoItem : public TimelineStillItem {
  float sourceStart;
  float sourceEnd;

  void printItem(std::ostream& stream, uint8_t indent = 0) const override {
    std::string indentStr = std::string(indent, ' ');
    stream << indentStr << "'" << name << "' from " << timelineStart << "s to " << timelineEnd << "s\n"
           << indentStr << "  - Path: " << srcPath << "\n"
           << indentStr << "  - Part taken from file: " << sourceStart << "s to " << sourceEnd << "s\n"
           << indentStr << "  - File size: ca. " << fileSizeKiB << "KiB\n"
           << indentStr << "  - Width x Height: " << srcSizePx.x << "px x " << srcSizePx.y << "px\n";
  }
};

} // Namespace mswmm

#endif
