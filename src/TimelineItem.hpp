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
#include <vector>
#include <ostream>


namespace mswmm {

struct size {
  size_t x;
  size_t y;

  bool operator!=(size const& rhs) const {
    if (rhs.x != x || rhs.y != y) {
      return true;
    }
    return false;
  }

  bool operator==(size const& rhs) const {
    return !(*this != rhs);
  }
};



struct TimelineItem {
  float timelineStart;
  float timelineEnd;
  std::vector<std::string> effects;

  virtual ~TimelineItem() = default;
  virtual void printItem(std::ostream& target, uint8_t indent = 0) const = 0;
  virtual void printEffects(std::ostream& target, uint8_t indent = 0) const {
    if (!effects.empty()) {
      target << std::string(indent*2, ' ') << "- Effects:\n";
      for (auto const& e: effects) {
        target << std::string(indent*3, ' ') << "- " << e << '\n';
      }
    }
  }
};



struct TimelineTitleItem : public TimelineItem {
  //std::string type;
  void printItem(std::ostream& target, uint8_t indent = 0) const override {
    std::string indentStr = std::string(indent, ' ');
    target << indentStr << "Title from " << timelineStart << "s to " << timelineEnd << "s\n";
    printEffects(target, indent);
  }
};



struct TimelineStillItem : public TimelineItem {
  std::string srcPath;
  std::string name;
  size_t fileSizeKiB;
  size srcSizePx;

  void printItem(std::ostream& target, uint8_t indent = 0) const override {
    std::string i1 = std::string(indent,   ' ');
    std::string i2 = std::string(indent*2, ' ');
    target << i1 << "'" << name << "' from " << timelineStart << "s to " << timelineEnd << "s\n"
           << i2 << "- Path: " << srcPath << "\n"
           << i2 << "- File size: ca. " << fileSizeKiB << "KiB\n"
           << i2 << "- Width x Height: " << srcSizePx.x << "px x " << srcSizePx.y << "px\n";
    printEffects(target, indent);
  }
};



struct TimelineVideoItem : public TimelineStillItem {
  float sourceStart;
  float sourceEnd;

  void printItem(std::ostream& target, uint8_t indent = 0) const override {
    std::string i1 = std::string(indent,   ' ');
    std::string i2 = std::string(indent*2, ' ');
    target << i1 << "'" << name << "' from " << timelineStart << "s to " << timelineEnd << "s\n"
           << i2 << "- Path: " << srcPath << "\n"
           << i2 << "- Part taken from file: " << sourceStart << "s to " << sourceEnd << "s\n"
           << i2 << "- File size: ca. " << fileSizeKiB << "KiB\n"
           << i2 << "- Width x Height: " << srcSizePx.x << "px x " << srcSizePx.y << "px\n";
    printEffects(target, indent);
  }
};



struct TimelineAudioItem : TimelineVideoItem {
  bool isMuted;
  bool fadesIn;
  bool fadesOut;
  float volume;

  void printItem(std::ostream& target, uint8_t indent = 0) const override {
    std::string i1 = std::string(indent,   ' ');
    std::string i2 = std::string(indent*2, ' ');
    target << i1 << "'" << name << "' from " << timelineStart << "s to " << timelineEnd << "s\n"
           << i2 << "- Path: " << srcPath << "\n"
           << i2 << "- Part taken from file: " << sourceStart << "s to " << sourceEnd << "s\n"
           << i2 << "- File size: ca. " << fileSizeKiB << "KiB\n";

    if (volume != 1) {
      target << i2 << "- Volume: " << volume << '\n';
    }
    if (isMuted) {
      target << i2 << "- Is muted\n";
    }

    std::string fade;
    if (fadesIn) {
      fade = "in";
    }
    if (fadesOut) {
      if (!fade.empty()) {
        fade += " &";
      }
      fade += " out";
    }
    if (!fade.empty()) {
      target << i2 << "- Fades " << fade << '\n';
    }
  }
};

} // Namespace mswmm

#endif
