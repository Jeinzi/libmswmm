# libmswmm
The goal of this project is to create a library and tools to read files created by Microsoft's Windows Movie Maker for ME, XP and Vista (file extension .MSWMM, up to version 6.0). It does not try to support Windows *Live* Movie Maker (file extension .wlmp, version 14.0 onward).

I have a lot of old project files and I guess many other people have too. I want to make it possible to view the contents of those projects so one can at least recreate them manually in a modern video editor. It also would be really nice to have converters/importers for Open Source video editors, or to generate an ffmpeg command.

## Current Capabilities
- Print project metadata
- Print paths to media files used in project
- Print project XML
- Print video timeline, but without detailed information on title sequences
- Generate an ffmpeg command to render the project. This currently works only in simple cases, when the project consist solely out of non overlapping video files. String substitutions on the source file paths are supported, for example to switch `\` to `/` and `@:MyPictures` to something like `/home/jeinzi/Pictures`. The title overlay and music timelines are completely ignored at the moment.

## Building
Just follow the commands in or execute make.sh.

Requirements:
- cmake
- a C++ compiler
- Qt

## License
This project is licensed under the GPLv2 or at your choice, any later version. The git submodule "compoundfilereader" is licensed under the MIT license. Files in the assets folder have various licenses, see *.license files for more information.

I chose the GPL because I want to provide the Free Software community and its video editors with a feature most proprietary products don't offer.
Nearly all Free Software video editors are licensed under the GPLv2 or 3, so there would be no problems to integrate this project into one of them. The only exception I found is [Pitivi](https://gitlab.gnome.org/GNOME/pitivi), which is distributed under the terms of the LGPL2.1 or later. As far as I understand it, integration into Pitivi would be possible, but the result would fall under the GPLv2 or later.
