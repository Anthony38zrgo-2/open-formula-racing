############################################################################
#
#   file        : robot.cmake
#   copyright   : (C) 2008 by Mart Kelder, 2010 by J.-P. Meuret
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

# @file     Robots-related macros
# @author   Mart Kelder

# Robot .def file generation for Windows builds with MSVC compilers
#  ROBOTNAME  : Name of the robot
#  DEF_FILE  : Target .def file  path-name
#  Other args  : Robot DLL Interface description, as a list of names of exported symbols,
#                 but with keywords shortcuts :
#                 - if empty, assumed keyword "LEGACY_MIN"
#                 - if "LEGACY_MIN", means the smallest legacy scheme interface  (TORCS style)
#                                    with only "${NAME}" exported
#                 - if "LEGACY", means the  complete legacy scheme interface,
#                                with "${NAME}" and "${NAME}Shut" exported
#                 - if "WELCOME", means the complete new scheme interface (Speed Dreams style),
#                                 with "moduleWelcome", "moduleInitialize" and "moduleTerminate"
#                                 exported
#                 - may be a list of above keywords for multi-scheme interface
#                 - may also be the raw list of symbols to export
MACRO(GENERATE_ROBOT_DEF_FILE ROBOTNAME DEF_FILE)

  # Build the real list of exported symbols from the given one (that may include shortcuts)
  #MESSAGE(STATUS "Generating ${DEF_FILE} for ${ROBOTNAME} robot")
  SET(SYMBOLS) # Initialize the list
  FOREACH(KEYSYM ${ARGN})
    IF(KEYSYM STREQUAL "LEGACY_MIN")
      LIST(APPEND SYMBOLS ${ROBOTNAME})
    ELSEIF(KEYSYM STREQUAL "LEGACY")
      LIST(APPEND SYMBOLS ${ROBOTNAME})
      LIST(APPEND SYMBOLS "${ROBOTNAME}Shut")
    ELSEIF(KEYSYM STREQUAL "WELCOME")
      LIST(APPEND SYMBOLS moduleWelcome moduleInitialize moduleTerminate)
    ELSE(KEYSYM STREQUAL "LEGACY_MIN")
      LIST(APPEND SYMBOLS ${KEYSYM})
    ENDIF(KEYSYM STREQUAL "LEGACY_MIN")
  ENDFOREACH(KEYSYM ${ARGN})

  # Clean duplicates
  LIST(REMOVE_DUPLICATES SYMBOLS)

  #MESSAGE(STATUS "Symbols: ${SYMBOLS}")

  # Build an acceptable string for the .def file from this symbol list
  SET(ROBOTSYMBOLS "")
  FOREACH(SYMBOL ${SYMBOLS})
    SET(ROBOTSYMBOLS "${ROBOTSYMBOLS}\n\t${SYMBOL}")
  ENDFOREACH(SYMBOL ${SYMBOLS})

  # Generate the .def file
  SET(ROBOT_NAME "${ROBOTNAME}")
  CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/robot.def.in.cmake ${DEF_FILE})

ENDMACRO(GENERATE_ROBOT_DEF_FILE ROBOTNAME DEF_FILE)

# Robot project definition (module build and install, without associated data)
# Args:
#  NAME       : Name of the robot
#  INTERFACE  : Robot Windows DLL Interface description (tells about exported symbols)
#                 See GENERATE_ROBOT_DEF_FILE macro.
#                 If not specified, defaults to "LEGACY_MIN" ; not used if MODULE used
#  SOURCES    : List of files to use as build sources if any ; not needed if MODULE used
#  VERSION    : The VERSION of the libraries to produce (robot and its clones) (def: $VERSION).
#  SOVERSION  : The SOVERSION of the libraries to produce (in the ldconfig meaning) (def: 0.0.0).
#               WARNING: Not taken into account for the moment : might not work with GCC 4.5 or +.
#
# Example:
#    ROBOT_MODULE(NAME simplix VERSION 3.0.5 SOVERSION 0.0.0
#                 INTERFACE LEGACY WELCOME simplix_trb1 simplix_ls1 simplix_36GP
#                 SOURCES simplix.cpp ...)
MACRO(ROBOT_MODULE)

  SET(RBM_SYNTAX "NAME,1,1,RBM_HAS_NAME,RBM_NAME")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "VERSION,0,1,RBM_HAS_VERSION,RBM_VERSION")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "SOVERSION,0,1,RBM_HAS_SOVERSION,RBM_SOVERSION")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "INTERFACE,0,-1,RBM_HAS_INTERFACE,RBM_INTERFACE")
  SET(RBM_SYNTAX ${RBM_SYNTAX} "SOURCES,0,-1,RBM_HAS_SOURCES,RBM_SOURCES")

  SPLIT_ARGN(${RBM_SYNTAX} ARGUMENTS ${ARGN})

  IF(NOT RBM_HAS_NAME OR NOT RBM_NAME)
    MESSAGE(FATAL_ERROR "Cannot build a robot module with no specified name")
  ENDIF()
  IF(NOT RBM_HAS_SOURCES OR NOT RBM_SOURCES)
    MESSAGE(FATAL_ERROR "Cannot build a robot module without sources / module to copy")
  ENDIF()
  IF(NOT RBM_HAS_VERSION OR NOT RBM_VERSION)
    SET(RBM_VERSION ${VERSION})
    MESSAGE(STATUS "No version specified for robot module ${RBM_NAME} ; using ${RBM_VERSION}")
  ENDIF()
  IF(NOT RBM_HAS_SOVERSION OR NOT RBM_SOVERSION)
    IF(UNIX)
      SET(RBM_SOVERSION 0.0.0)
      MESSAGE(STATUS "No so-version specified for robot module ${RBM_NAME} ; using ${RBM_SOVERSION}")
    ENDIF()
  ENDIF()

  PROJECT("robot_${RBM_NAME}")

  ADD_INTERFACE_INCLUDEDIR()
  ADD_SDLIB_INCLUDEDIR(learning math portability robottools tgf)
  ADD_PLIB_INCLUDEDIR()

  # DLL export stuff under Windows (through a .def file or __declspec pragmas)
  IF(WIN32)
    # If an interface is specified, use the old way.
    IF(RBM_HAS_INTERFACE AND RBM_INTERFACE)
      IF(MSVC)
        # For MSVC compilers, generate / add a .def file for legacy / welcome interface.
        SET(ROBOT_DEF_FILE ${CMAKE_CURRENT_BINARY_DIR}/${RBM_NAME}_gen.def)
        GENERATE_ROBOT_DEF_FILE(${RBM_NAME} ${ROBOT_DEF_FILE} ${RBM_INTERFACE})
        SET(RBM_SOURCES ${RBM_SOURCES} ${ROBOT_DEF_FILE})
      ENDIF()
    # If no interface is specified, assume it's the new modern one.
    ELSE()
      # For any Windows compiler, use __declspec pragmas.
      ADD_DEFINITIONS(-DROBOT_DLL)
    ENDIF()
  ENDIF(WIN32)

  # Disable developer warning
  IF (COMMAND cmake_policy)
    CMAKE_POLICY(SET CMP0003 NEW)
  ENDIF(COMMAND cmake_policy)

  # Ignore some run-time libs to avoid MSVC link-time warnings and sometimes even crashes.
  IF(MSVC)
      SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib")
  ENDIF(MSVC)

  # The robot module is actually a module.
  SD_ADD_LIBRARY(${RBM_NAME} ROBOT ${RBM_SOURCES})

  # Customize shared library versions.
  IF(UNIX) # Use ldconfig version naming scheme + no "lib" prefix under Linux
    # Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose)
    #SET_TARGET_PROPERTIES(${RBM_NAME} PROPERTIES VERSION ${RBM_VERSION})
    #SET_TARGET_PROPERTIES(${RBM_NAME} PROPERTIES SOVERSION ${RBM_SOVERSION})
  ELSE()
    SET_TARGET_PROPERTIES(${RBM_NAME} PROPERTIES VERSION ${RBM_VERSION})
  ENDIF()

  # Link/Run-time dependencies
  ADD_PLIB_LIBRARY(${RBM_NAME} sg)

  ADD_SDLIB_LIBRARY(${RBM_NAME} portability tgf robottools)

  # Install target robot module shared library
  SD_INSTALL_FILES(LIB drivers/${RBM_NAME} TARGETS ${RBM_NAME})
ENDMACRO(ROBOT_MODULE)
