cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(libmswmm VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Define colors.
if(NOT WIN32)
  string(ASCII 27 Esc)
  set(ColorReset "${Esc}[m")
  set(ColorBold  "${Esc}[1m")
  set(Red         "${Esc}[31m")
  set(Green       "${Esc}[32m")
  set(Yellow      "${Esc}[33m")
  set(Blue        "${Esc}[34m")
  set(Magenta     "${Esc}[35m")
  set(Cyan        "${Esc}[36m")
  set(White       "${Esc}[37m")
  set(BoldRed     "${Esc}[1;31m")
  set(BoldGreen   "${Esc}[1;32m")
  set(BoldYellow  "${Esc}[1;33m")
  set(BoldBlue    "${Esc}[1;34m")
  set(BoldMagenta "${Esc}[1;35m")
  set(BoldCyan    "${Esc}[1;36m")
  set(BoldWhite   "${Esc}[1;37m")
endif()

# Prevent compilation in-source.
if(${CMAKE_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    Message(" ")
    Message(FATAL_ERROR "${BoldRed}Source and build directories are the same.
Create an empty build directory, change into it and re-invoke cmake.${ColorReset}")
endif()

# Qt.
find_package(Qt6 REQUIRED COMPONENTS Core Xml)
qt_standard_project_setup()

# Other source files.
include_directories("src/")
include_directories("compoundfilereader/src/include/")
file(GLOB TOOLSRC "src/*.cpp")

# Compiler options.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s")

# Create output files.
add_executable(mswmm-tool ${TOOLSRC})
target_link_libraries(mswmm-tool PRIVATE Qt6::Core)
target_link_libraries(mswmm-tool PRIVATE Qt6::Xml)
