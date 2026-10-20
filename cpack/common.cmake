#  Speed Dreams, a free and open source motorsport simulator.
#  Copyright (C) 2019 Joe Thompson, 2025 Xavier Del Campo Romero
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.

set(CPACK_PACKAGE_NAME "speed-dreams")
set(CPACK_PACKAGE_VENDOR "The Speed Dreams Team")
set(CPACK_PACKAGE_CONTACT "The Speed Dreams Team <contact@speed-dreams.net>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}${CMAKE_PROJECT_VERSION_TWEAK}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src")
set(CPACK_PACKAGE_DESCRIPTION
"Speed Dreams is a free and open source motorsport simulator. Originally a
fork of the TORCS project, it has evolved into a higher level of maturity,
featuring realistic physics with tens of high-quality cars and tracks to
choose from.

Speed Dreams features multiple categories from all racing eras (36GP
and 67GP, Supercars, Long Day Series 1/2, and many more), 4
computer-controlled driver implementations, flexible race configuration
(real-time weather conditions, multi-class races, etc.), as well as a
master server to compare your best lap times against other players."
)

if(NOT SD_ASSUME_DATADIR)
    set(CPACK_PACKAGE_ICON "${SD_DATADIR_ABS}/data/img/header.bmp")
endif()

include(${CMAKE_CURRENT_LIST_DIR}/debian.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/dmg.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/nsis.cmake)
