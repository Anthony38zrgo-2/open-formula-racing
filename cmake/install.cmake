############################################################################
#
#   file        : install.cmake
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

# @file     CMake install macros
# @author   Mart Kelder, J.-P. Meuret


# Get the real pathname of a target (taking care of the MSVC configuration-driven variables).
# Args :
#  TGT_NAME Name of the target
#  Name of the target variable for the output path-name
MACRO(_GET_TARGET_REAL_PATHNAME TGT_NAME VAR_PATHNAME)

    GET_TARGET_PROPERTY(${VAR_PATHNAME} ${TGT_NAME} LOCATION)
    MESSAGE(STATUS "GET_TARGET_REAL_PATHNAME(${TGT_NAME})=${${VAR_PATHNAME}}")
    IF(MSVC)
      STRING(REPLACE "$(OutDir)" "${CMAKE_INSTALL_CONFIG_NAME}" ${VAR_PATHNAME} ${${VAR_PATHNAME}})
      STRING(REPLACE "$(ConfigurationName)" "${CMAKE_INSTALL_CONFIG_NAME}" ${VAR_PATHNAME} ${${VAR_PATHNAME}})
      STRING(REPLACE "$(Configuration)" "${CMAKE_INSTALL_CONFIG_NAME}" ${VAR_PATHNAME} ${${VAR_PATHNAME}})
    ENDIF(MSVC)

ENDMACRO(_GET_TARGET_REAL_PATHNAME TGT_NAME VAR_PATHNAME)

# Lib/Bin/Include files installation
# Note: Missing files will be skipped if not there and OPTION_CHECK_CONTENTS is Off.
# Args:
#  LIB      : Lib subdirectory where to install specified files/targets
#  BIN      : If present, instructs to install specified files/targets in the bin dir
#  INCLUDE  : Include subdirectory where to install specified files
#  USER     : User settings subdirectory where to install/update specified data files at run-time
#  PREFIX   : Prefix to use to get source path for files specified in FILES
#  FILES    : Files to install (see PREFIX)
#  TARGETS  : Targets to install
# Examples:
#  SD_INSTALL_FILES(LIB drivers/bt TARGETS bt.so)
#     Installs bt.so in ${prefix}/${SD_LIBDIR}/drivers/bt
#  SD_INSTALL_FILES(BIN TARGETS speed-dreams)
#     Installs the speed-dreams target in ${prefix}/${SD_BINDIR}
#  SD_INSTALL_FILES(MAN man6 PREFIX ${CMAKE_SOURCE_DIR}/doc/man FILES sd2-menuview.6)
#     Installs ${CMAKE_SOURCE_DIR}/doc/man/sd2-menuview.6 in ${prefix}/${SD_MANDIR}/man6
MACRO(SD_INSTALL_FILES)

  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "LIB,1,1,IS_LIB,LIB_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "BIN,0,0,IS_BIN,_")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "INCLUDE,0,1,IS_INCLUDE,INCLUDE_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "MAN,1,1,IS_MAN,MAN_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "USER,1,1,IS_USER,USER_PATH")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "PREFIX,0,1,HAS_PREFIX,PREFIX")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "FILES,0,-1,HAS_FILES,FILES")
  SET(SD_INSTALL_FILES_SYNTAX ${SD_INSTALL_FILES_SYNTAX} "TARGETS,0,-1,HAS_TARGETS,TARGETS")

  SPLIT_ARGN(${SD_INSTALL_FILES_SYNTAX} ARGUMENTS ${ARGN})

  #MESSAGE(STATUS "  SD_INSTALL_FILES: LIB=${IS_LIB}:${LIB_PATH} BIN=${IS_BIN} INCLUDE=${IS_INCLUDE}:${INCLUDE_PATH} DATA=${IS_DATA}:${DATA_PATH} MAN=${IS_MAN}:${MAN_PATH} USER=${IS_USER}:${USER_PATH} TARGETS=${HAS_TARGETS}:${TARGETS} FILES=${HAS_FILES}:${FILES}")

  # Fix/Check argument syntax / values
  IF(NOT USER_PATH)
    SET(IS_USER FALSE)
  ENDIF()
  IF(NOT LIB_PATH)
    SET(IS_LIB FALSE)
  ENDIF()
  IF(NOT MAN_PATH)
    SET(IS_MAN FALSE)
  ENDIF()
  IF(NOT PREFIX)
    SET(HAS_PREFIX FALSE)
  ENDIF()
  IF(NOT FILES)
    SET(HAS_FILES FALSE)
  ENDIF()
  IF(NOT TARGETS)
    SET(HAS_TARGETS FALSE)
  ENDIF()

  IF(IS_LIB OR IS_BIN OR IS_INCLUDE OR IS_MAN)
    IF(HAS_PREFIX)
      IF(NOT HAS_FILES)
        MESSAGE(FATAL_ERROR "SD_INSTALL_FILES: Expected FILES when PREFIX keyword is present")
      ENDIF()
    ENDIF()
  ELSE()
    MESSAGE(FATAL_ERROR "SD_INSTALL_FILES: Expected 1 and only 1 LIB, BIN, INCLUDE or MAN keyword")
  ENDIF()

  # Compute destination sub-dir
  IF(IS_LIB)
    SET(DEST1 ${SD_LIBDIR})
    SET(DEST2 ${LIB_PATH})
  ELSEIF(IS_BIN)
    SET(DEST1 ${SD_BINDIR})
    SET(DEST2 "")
  ELSEIF(IS_INCLUDE)
    SET(DEST1 ${SD_INCLUDEDIR})
    SET(DEST2 ${INCLUDE_PATH})
  ELSEIF(IS_MAN)
    SET(DEST1 ${SD_MANDIR})
    SET(DEST2 ${MAN_PATH})
  ENDIF()

  IF(DEST2 STREQUAL "" OR DEST2 STREQUAL "/")
    SET(DEST2 "")
    SET(DEST_ALL "${DEST1}")
  ELSE()
    SET(DEST_ALL "${DEST1}/${DEST2}")
  ENDIF()

  # Prepend prefix to files if specified.
  SET(REAL_FILES) # Reset the list (remember, it's a CMakeLists.txt global variable :-()
  IF(HAS_FILES)
    SET(_FILES) # Same.
    FOREACH(FILE ${FILES})
      #MESSAGE(STATUS "SD_INSTALL_FILES: ${FILE}")
      IF(HAS_PREFIX)
        SET(_FILE ${PREFIX}/${FILE})
      ELSE()
        SET(_FILE ${FILE})
      ENDIF()
      # Contents check for non-generated files if specified.
      IF(NOT IS_ABSOLUTE ${_FILE})
        SET(_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${_FILE}")
      ENDIF()
      IF(IS_LIB OR IS_BIN OR EXISTS ${_FILE} OR OPTION_CHECK_CONTENTS)
        LIST(APPEND REAL_FILES ${_FILE})
        LIST(APPEND _FILES ${FILE})
      ELSE()
        IF(IS_ABSOLUTE ${_FILE}) # Make message less long : remove useless source dir path.
          STRING(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" _FILE "${_FILE}")
        ENDIF()
        MESSAGE(STATUS "Note : Won't install missing file ${_FILE}")
      ENDIF()
    ENDFOREACH()
    SET(FILES ${_FILES})
  ENDIF()

  # Install files
  #MESSAGE(STATUS "REAL_FILES '${REAL_FILES}' DESTINATION '${DEST_ALL}'")
  IF(REAL_FILES)
    INSTALL(FILES ${REAL_FILES} DESTINATION ${DEST_ALL})
  ENDIF()

  # Install targets
  IF(HAS_TARGETS)

    INSTALL(TARGETS ${TARGETS} DESTINATION ${DEST_ALL})

  ENDIF()
ENDMACRO(SD_INSTALL_FILES)
