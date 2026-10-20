############################################################################
#
#   file        : macros.cmake
#   copyright   : (C) 2008 by Mart Kelder
#   web         : www.speed-dreams.org
#
############################################################################

############################################################################
#                                                                          #
#   This program is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by   #
#   the Free Software Foundation; either version 2 of the License, or      #
#   (at your option) any later version.                                    #
#                                                                          #
############################################################################

# @file     Main CMake configuration file (to be included in every CMakeLists.txt)
# @author   Mart Kelder

#MESSAGE(STATUS "Processing ${CMAKE_CURRENT_SOURCE_DIR} ...")

# Setup the install prefix.
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  IF(WIN32)
    SET(CMAKE_INSTALL_PREFIX "installed-build" CACHE PATH "Prefix prepended to install directories" FORCE)
  ELSEIF(APPLE)
    SET(CMAKE_INSTALL_PREFIX "speed-dreams-2.app" CACHE PATH "Prefix prepended to install directories" FORCE)
  ELSE()
    SET(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Prefix prepended to install directories" FORCE)
  ENDIF()
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

# Macros arg list parsing tools.
IF(NOT _ALREADY_DONE)
  INCLUDE(${CMAKE_CURRENT_LIST_DIR}/splitargn.cmake)
ENDIF(NOT _ALREADY_DONE)

# Determine the default value of the user settings folder.
IF(WIN32)
  SET(SD_LOCALDIR "~/speed-dreams-2.settings" CACHE STRING "Where the user settings files should go")
ELSEIF(HAIKU)
  SET(SD_LOCALDIR "~/config/settings/speed-dreams" CACHE STRING "Where the user settings files should go")
ELSE(WIN32) #UNIX
  SET(SD_LOCALDIR "~/.speed-dreams-2" CACHE STRING "Where the user settings files should go")
ENDIF(WIN32)

# Determine the default value of the tools executable file prefix.
SET(SD_TOOLS_EXECPREFIX "sd2-" CACHE STRING "Prefix for the tools executable names")
MARK_AS_ADVANCED(SD_TOOLS_EXECPREFIX)

IF(WIN32)
  SET(SD_BINDIR ${CMAKE_INSTALL_BINDIR} CACHE PATH "Place where the executables should go")
ELSE()
  SET(SD_BINDIR games CACHE PATH "Place where the executables should go")
ENDIF()

SET(SD_LIBDIR ${CMAKE_INSTALL_LIBDIR}/games/speed-dreams-2 CACHE PATH "Place where the libraries should go")
SET(SD_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR}/speed-dreams-2 CACHE PATH "Place where the include files should go")
SET(SD_MANDIR ${CMAKE_INSTALL_MANDIR} CACHE PATH "Place where the manual pages should go")

STRING(REGEX REPLACE "^(.*[^/])/*$" "\\1" SD_LOCALDIR_TMP ${SD_LOCALDIR})
SET(SD_LOCALDIR ${SD_LOCALDIR_TMP})

# Configuration options macros.
INCLUDE(${CMAKE_CURRENT_LIST_DIR}/options.cmake)

# Robots-related macros.
INCLUDE(${CMAKE_CURRENT_LIST_DIR}/robot.cmake)

# Robots-related macros.
INCLUDE(${CMAKE_CURRENT_LIST_DIR}/install.cmake)

# Internal dependencies macros (includes and libs).
INCLUDE(${CMAKE_CURRENT_LIST_DIR}/internaldeps.cmake)

# 3rd party dependencies macros (includes and libs).
INCLUDE(${CMAKE_CURRENT_LIST_DIR}/thirdpartydeps.cmake)

# Use as a replacement of native ADD_DIRECTORY if the target folder may be optional
# (if it is actually not there, and OPTION_CHECK_CONTENTS is Off,
#  then the build will continue with a simple status message).
MACRO(SD_ADD_SUBDIRECTORY DIR_PATH)

  IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${DIR_PATH} OR OPTION_CHECK_CONTENTS)
    ADD_SUBDIRECTORY(${DIR_PATH})
  ELSE()
    MESSAGE(STATUS "Note : Won't build missing dir. ${DIR_PATH}")
  ENDIF()

ENDMACRO(SD_ADD_SUBDIRECTORY PATH)

# Replacement of standard ADD_EXECUTABLE command (same interface).
MACRO(SD_ADD_EXECUTABLE TARGET_NAME)

  # Standard ADD_EXECUTABLE command.
  ADD_EXECUTABLE(${TARGET_NAME} ${ARGN})

  # Change target location (for running in build-tree without installing).
  SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_BINDIR}")

  SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY ${_TGT_DIR})

  IF(MSVC)

    FOREACH(_CFG ${CMAKE_CONFIGURATION_TYPES})
      STRING(TOUPPER ${_CFG} _CFG)
      SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
                            RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${_TGT_DIR}")
    ENDFOREACH()

  ENDIF(MSVC)

  SET_PROPERTY(GLOBAL APPEND PROPERTY SD_EXE_LIST "${SD_BINDIR}/${TARGET_NAME}${CMAKE_EXECUTABLE_SUFFIX}")

ENDMACRO(SD_ADD_EXECUTABLE TARGET_NAME)


# Replacement of standard ADD_LIBRARY command,
# in order to take care of :
# * changing target location, for running in build-tree without installing,
# * changing target name for modules and robot DLLs (no "lib" prefix).
# Nearly same behaviour as standard ADD_LIBRARY, but :
# * more library types (possible values for TARGET_TYPE arg) :
#   - STATIC, SHARED, MODULE : no change,
#   - ROBOT : same as MODULE for standard ADD_LIBRARY.
# * TARGET_TYPE type arg is mandatory (no default).
MACRO(SD_ADD_LIBRARY TARGET_NAME TARGET_TYPE)

  #MESSAGE(STATUS "SD_ADD_LIBRARY : TARGET_NAME = ${TARGET_NAME} TARGET_TYPE = ${TARGET_TYPE} ARGN = ${ARGN}")

  # Standard ADD_EXECUTABLE command.
  IF(${TARGET_TYPE} STREQUAL "ROBOT")
    ADD_LIBRARY(${TARGET_NAME} MODULE ${ARGN})
  ELSE()
    ADD_LIBRARY(${TARGET_NAME} ${TARGET_TYPE} ${ARGN})
  ENDIF()

  # Determine target location (for running in build-tree without installing).
  IF(${TARGET_TYPE} STREQUAL "SHARED")
    IF(WIN32)
      SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_BINDIR}")
    ELSE()
      SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_LIBDIR}/lib")
    ENDIF()

  ELSEIF(${TARGET_TYPE} STREQUAL "MODULE")
    SET(_TGT_LOC ${CMAKE_CURRENT_SOURCE_DIR})
    GET_FILENAME_COMPONENT(_TGT_TYPE ${_TGT_LOC} PATH)
    GET_FILENAME_COMPONENT(_TGT_TYPE ${_TGT_TYPE} NAME)
    SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_LIBDIR}/modules/${_TGT_TYPE}")
    SET_PROPERTY(GLOBAL APPEND PROPERTY SD_MODULE_LIST "${SD_LIBDIR}/modules/${_TGT_TYPE}/${TARGET_NAME}${CMAKE_SHARED_MODULE_SUFFIX}")
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES SD_TYPE "MODULE")

  ELSEIF(${TARGET_TYPE} STREQUAL "ROBOT")

    SET(_TGT_DIR "${CMAKE_BINARY_DIR}/${SD_LIBDIR}/drivers/${TARGET_NAME}")
    SET_PROPERTY(GLOBAL APPEND PROPERTY SD_ROBOT_LIST "${TARGET_NAME}")

  ELSEIF(NOT ${TARGET_TYPE} STREQUAL "STATIC")

  ENDIF()

  # Change target location (for running in build-tree without installing).
  SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY "${_TGT_DIR}"
                        LIBRARY_OUTPUT_DIRECTORY "${_TGT_DIR}")

  IF(MSVC)

    FOREACH(_CFG ${CMAKE_CONFIGURATION_TYPES})
      STRING(TOUPPER ${_CFG} _CFG)
      SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
                            RUNTIME_OUTPUT_DIRECTORY_${_CFG} "${_TGT_DIR}"
                            LIBRARY_OUTPUT_DIRECTORY_${_CFG} "${_TGT_DIR}")
    ENDFOREACH()

  ENDIF(MSVC)

  #MESSAGE(STATUS "SD_ADD_LIBRARY : _TGT_DIR = ${_TGT_DIR}")

  # No prefix for module and robot DLLs.
  IF(${TARGET_TYPE} STREQUAL "MODULE" OR ${TARGET_TYPE} STREQUAL "ROBOT")

    IF(UNIX OR MINGW)
      SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES PREFIX "")
    ENDIF(UNIX OR MINGW)

  ENDIF()

ENDMACRO(SD_ADD_LIBRARY TARGET_NAME TARGET_TYPE)

ADD_COMPILE_DEFINITIONS(HAVE_CONFIG_H)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

# Add non-default compile options.
ADD_SD_COMPILE_OPTIONS()

# A useful variable for things that only need to be done once
# (macros.cmake is actually included by every CMakeLists.txt,
#  in order one can run 'cmake .' everywhere in the source tree,
#  but the bad side effect if that it is thus often included
#  _multiple_ times by every CMakeLists.txt).
IF(NOT _ALREADY_DONE)
  SET(_ALREADY_DONE TRUE)
ENDIF(NOT _ALREADY_DONE)
