INCLUDE(../../../cmake/macros.cmake)

#PROJECT(csnetworking)

SET(SD_CSNETWORK_SOURCES csnetwork.cpp csserver.cpp csclient.cpp csrobotxml.cpp cspack.cpp)
SET(SD_CSNETWORK_HEADERS csnetwork.h csrobotxml.h cspack.h)

#disable developer warning
if (COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

# Note: Headers needed for having them available in IDEs.
#TODO: Maje this a real (dynamically loadable) module, or move it to standard src/libs !
SD_ADD_LIBRARY(csnetworking SHARED ${SD_CSNETWORK_SOURCES} ${SD_CSNETWORK_HEADERS})

target_include_directories(csnetworking PUBLIC ${CMAKE_CURRENT_LIST_DIR})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose)
#SET_TARGET_PROPERTIES(csnetworking PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_INTERFACE_INCLUDEDIR()

ADD_SDL2_INCLUDEDIR()

ADD_SDLIB_INCLUDEDIR(csnetworking portability tgf math)

ADD_PLIB_INCLUDEDIR()
ADD_ENET_INCLUDEDIR()
ADD_PLIB_LIBRARY(csnetworking ul sg)
ADD_ENET_LIBRARY(csnetworking)
ADD_SDL2_LIBRARY(csnetworking)

IF(WIN32)
	TARGET_LINK_LIBRARIES(csnetworking winmm ws2_32)
ENDIF(WIN32)

IF(MSVC)
	# Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
	SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG
		"${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib")
	SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE
		"${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)


ADD_SDLIB_LIBRARY(csnetworking portability tgf)

IF(WIN32)
	SD_INSTALL_FILES(BIN TARGETS csnetworking)
ELSE(WIN32)
	SD_INSTALL_FILES(LIB lib TARGETS csnetworking)
ENDIF(WIN32)
