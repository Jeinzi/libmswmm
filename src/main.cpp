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
#include <string>
#include <cstring>
#include <iostream>

#include "Project.hpp"



int main(int argc, char** argv) {
  if (argc <= 2) {
    std::cout << "Usage: ./programname command path/to/file.MSWMM\n";
    std::cout << "        where command = info|xml|ffmpeg" << std::endl;
    return 1;
  }

  mswmm::Project project(argv[2]);

  if (strcmp(argv[1], "xml") == 0) {
    project.printXml(std::cout);
  }
  else if (strcmp(argv[1], "info") == 0) {
    std::cout << "Metadata:\n";
    project.printMetadata(std::cout, 2);
    std::cout << "Files used in project:\n";
    project.printFiles(std::cout, 2);
    std::cout << "Video timeline:\n";
    project.printVideoTimeline(std::cout, 2);
  }
  else if (strcmp(argv[1], "ffmpeg") == 0) {
    std::vector<std::pair<std::string, std::string>> substitutions;
    substitutions.emplace_back(std::pair("\\", "/"));
    substitutions.emplace_back(std::pair("@:MyPictures", "/home/jeinzi/Bilder"));
    std::cout << project.generateFfmpegCommand(substitutions) << std::endl;
  }
  else {
    std::cout << "Command not known." << std::endl;
    return 1;
  }

  return 0;
}
