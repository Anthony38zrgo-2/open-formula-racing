# Copyright (C) 2024 Xavier Del Campo Romero
# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

find_path(rhash_INCLUDE_DIRS
    NAMES
        rhash.h
        rhash_torrent.h
    HINTS
        ENV rhash_PATH
    PATH_SUFFIXES
        include
)

find_library(rhash_LIBRARIES
    NAMES
        rhash
    HINTS
        ENV rhash_PATH
    PATH_SUFFIXES
        lib
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(rhash
    REQUIRED_VARS
        rhash_LIBRARIES rhash_INCLUDE_DIRS
)

if(rhash_FOUND)
    if(NOT TARGET rhash::rhash)
        add_library(rhash::rhash INTERFACE IMPORTED)
        set_target_properties(rhash::rhash PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${rhash_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${rhash_LIBRARIES}")
    endif()
endif()
