# Copyright (C) 2024 Xavier Del Campo Romero
# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

find_path(minizip_INCLUDE_DIRS
    NAMES
        minizip/crypt.h
        minizip/ioapi.h
        minizip/mztools.h
        minizip/unzip.h
        minizip/zip.h
    HINTS
        ENV minizip_PATH
    PATH_SUFFIXES
        include
)

find_library(minizip_LIBRARIES
    NAMES
        minizip
    HINTS
        ENV minizip_PATH
    PATH_SUFFIXES
        lib
)

set(regex_path "${minizip_INCLUDE_DIRS}/minizip/unzip.h")

if(minizip_INCLUDE_DIRS AND EXISTS ${regex_path})
    set(version_regex "^[ \t]+Version ([0-9\.]+).+$")
    file(STRINGS ${regex_path} minizip_VERSION_LINE REGEX ${version_regex})
    string(REGEX REPLACE ${version_regex} "\\1" minizip_VERSION "${minizip_VERSION_LINE}")
    unset(minizip_VERSION_LINE)
    unset(version_regex)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(minizip
    REQUIRED_VARS
        minizip_LIBRARIES minizip_INCLUDE_DIRS
    VERSION_VAR
        minizip_VERSION
)

if(minizip_FOUND)
    if(NOT TARGET minizip::minizip)
        add_library(minizip::minizip INTERFACE IMPORTED)
        set_target_properties(minizip::minizip PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${minizip_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${minizip_LIBRARIES}")
    endif()
endif()
