# This file is part of the OregonCore Project. See AUTHORS file for Copyright information
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Set projectname (must be done AFTER setting configurationtypes)
# We require CMake >= 2.8

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

project(OregonCore)

cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0063 NEW)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_C_STANDARD          17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# set macro-directory
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

# build in preferred mode
if( NOT CMAKE_BUILD_TYPE )
    if (WITH_COREDEBUG)
        set(CMAKE_BUILD_TYPE "Debug")
    else()
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()

include(CheckCXXSourceRuns)
include(CheckIncludeFiles)

# set default buildoptions and print them
include(cmake/options.cmake)

# turn off PCH totally if enabled (hidden setting, mainly for devs)
if( NOPCH )
  set(USE_COREPCH 0)
  set(USE_SCRIPTPCH 0)
endif()

# platform detection
include(CheckPlatform)

# basic package searching and setup
#find_package(ACE REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(MySQL REQUIRED)

if( UNIX )
   find_package(Curses)
   find_package(ZLIB)
   find_package(BZip2)
   find_package(Binutils)
   find_package(SysInfo)
endif()

# Find revision ID and hash of the sourcetree
include(cmake/genrev.cmake)

# doxygen support
include(cmake/doxygen.cmake)

# print out the results before continuing
include(cmake/showoptions.cmake)

# add dependencies
add_subdirectory(dep)

# add core sources
add_subdirectory(src)
