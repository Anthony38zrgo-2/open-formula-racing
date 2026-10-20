############################################################################
#
#   file        : internaldeps.cmake
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

# @file     Internal dependencies (include and libs)
# @author   Mart Kelder, J.-P. Meuret

# SD include dirs macros.
MACRO(ADD_INTERFACE_INCLUDEDIR)

ENDMACRO(ADD_INTERFACE_INCLUDEDIR)

MACRO(ADD_SDLIB_INCLUDEDIR)

ENDMACRO(ADD_SDLIB_INCLUDEDIR)

# SD libraries macro.
MACRO(ADD_SDLIB_LIBRARY TARGET)

  #MESSAGE(STATUS "ADD_SDLIB_LIBRARY : TARGET = ${TARGET}")
  #MESSAGE(STATUS "ADD_SDLIB_LIBRARY : ARGN = ${ARGN}")

  SET(SDLIB_OPTIONAL FALSE)
  SET(SDLIB_STATIC FALSE)
  SET(SDLIB_TARGET_SUFFIX "")

  FOREACH(SDLIB_LIB ${ARGN})
    IF(${SDLIB_LIB} STREQUAL "OPTIONAL")
      SET(SDLIB_OPTIONAL TRUE)
    ENDIF(${SDLIB_LIB} STREQUAL "OPTIONAL")
    IF(${SDLIB_LIB} STREQUAL "STATIC")
      SET(SDLIB_STATIC TRUE)
      SET(SDLIB_TARGET_SUFFIX "_static")
    ENDIF(${SDLIB_LIB} STREQUAL "STATIC")
  ENDFOREACH(SDLIB_LIB ${ARGN})

  FOREACH(SDLIB_LIB ${ARGN})

    SET(SDLIB_IGNORE TRUE)
    IF(NOT UNIX)
      SET(SDLIB_IGNORE FALSE)
    ELSEIF(NOT SDLIB_LIB STREQUAL "ssggraph" AND NOT SDLIB_LIB STREQUAL "osggraph" AND NOT SDLIB_LIB STREQUAL "track")
      SET(SDLIB_IGNORE FALSE)
    ENDIF(NOT UNIX)
    IF(SDLIB_LIB STREQUAL "OPTIONAL" OR SDLIB_LIB STREQUAL "STATIC")
      SET(SDLIB_IGNORE TRUE) #Ignore: not a real target
    ENDIF(SDLIB_LIB STREQUAL "OPTIONAL" OR SDLIB_LIB STREQUAL "STATIC")

    IF(SDLIB_LIB STREQUAL "txml" AND OPTION_3RDPARTY_EXPAT)
      SET(SDLIB_IGNORE TRUE) #Ignore: Use Expat
    ENDIF(SDLIB_LIB STREQUAL "txml" AND OPTION_3RDPARTY_EXPAT)

	IF(SDLIB_LIB STREQUAL "ephemeris" OR SDLIB_LIB STREQUAL "STATIC")
      SET(SDLIB_IGNORE TRUE)
    ENDIF(SDLIB_LIB STREQUAL "ephemeris" OR SDLIB_LIB STREQUAL "STATIC")

    IF(NOT SDLIB_IGNORE)

      SET(SDLIB_LIBRARIES ${SDLIB_LIBRARIES} ${SDLIB_LIB}${SDLIB_TARGET_SUFFIX})
      #MESSAGE(STATUS "ADD_SDLIB_LIBRARY : SDLIB_LIBRARIES = ${SDLIB_LIBRARIES}")

    ENDIF(NOT SDLIB_IGNORE)

  ENDFOREACH(SDLIB_LIB ${SDLIB_LIBS})

  #MESSAGE(STATUS "TARGET_LINK_LIBRARIES(${TARGET} ${SDLIB_LIBRARIES})")
  TARGET_LINK_LIBRARIES(${TARGET} ${SDLIB_LIBRARIES})
  # FIX: most libraries require this interface library.
  TARGET_LINK_LIBRARIES(${TARGET} interfaces)

ENDMACRO(ADD_SDLIB_LIBRARY TARGET)
