#ifndef _TIMELINEITEM_HPP
#define _TIMELINEITEM_HPP

#include <string>

namespace mswmm {


struct size {
  size_t x;
  size_t y;
};



struct TimelineItem {
  std::string srcPath;
  std::string name;
  size_t fileSizeKiBi;
  size srcSizePx;
  float timelineStart;
  float timelineEnd;
  float sourceStart;
  float sourceEnd;
};


} // Namespace mswmm

#endif
