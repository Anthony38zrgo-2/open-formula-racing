INCLUDE(../../../../cmake/macros.cmake)

SET(MS mainscreens)
SET(MAINSCREENS_SOURCES ${MS}/splash.cpp ${MS}/mainmenu.cpp
                        ${MS}/optionsmenu.cpp ${MS}/creditsmenu.cpp ${MS}/exitmenu.cpp
                        ${MS}/asset.cpp
                        ${MS}/assets.cpp
                        ${MS}/confirmmenu.cpp
                        ${MS}/downloadservers.cpp
                        ${MS}/downloadsmenu.cpp
                        ${MS}/entry.cpp
                        ${MS}/infomenu.cpp
                        ${MS}/repomenu.cpp
                        ${MS}/sha256.cpp
                        ${MS}/sink.cpp
                        ${MS}/thumbnail.cpp
                        ${MS}/unzip.cpp
                        ${MS}/writebuf.cpp
                        ${MS}/writefile.cpp)
SET(MAINSCREENS_HEADERS ${MS}/splash.h ${MS}/mainmenu.h
                        ${MS}/optionsmenu.h ${MS}/creditsmenu.h ${MS}/exitmenu.h
                        ${MS}/asset.h
                        ${MS}/assets.h
                        ${MS}/confirmmenu.h
                        ${MS}/downloadservers.h
                        ${MS}/downloadsmenu.h
                        ${MS}/entry.h
                        ${MS}/infomenu.h
                        ${MS}/repomenu.h
                        ${MS}/sha256.h
                        ${MS}/sink.h
                        ${MS}/thumbnail.h
                        ${MS}/unzip.h
                        ${MS}/writebuf.h
                        ${MS}/writefile.h)

IF(MSVC)
    # errno.cpp should only be included with MSVC
    # (it fails to compile when added with GCC)
    SET(MAINSCREENS_SOURCES ${MAINSCREENS_SOURCES} ${MS}/errno.cpp)
ENDIF(MSVC)

SET(CS confscreens)
SET(CONFSCREENS_SOURCES ${CS}/playerconfig.cpp ${CS}/controlconfig.cpp
                        ${CS}/joystickconfig.cpp ${CS}/mouseconfig.cpp
                        ${CS}/joy2butconfig.cpp
                        ${CS}/displayconfig.cpp ${CS}/graphconfig.cpp ${CS}/openglconfig.cpp
                        ${CS}/monitorconfig.cpp ${CS}/advancedgraphconfig.cpp
                        ${CS}/simuconfig.cpp ${CS}/soundconfig.cpp
                        ${CS}/hostsettingsmenu.cpp ${CS}/forcefeedbackconfig.cpp
                        ${CS}/languageconfig.cpp)
SET(CONFSCREENS_HEADERS ${CS}/confscreens.h ${CS}/advancedgraphconfig.h
                        ${CS}/playerconfig.h ${CS}/controlconfig.h
                        ${CS}/joystickconfig.h ${CS}/mouseconfig.h
                        ${CS}/joy2butconfig.h ${CS}/monitorconfig.h
                        ${CS}/displayconfig.h ${CS}/graphconfig.h ${CS}/openglconfig.h
                        ${CS}/simuconfig.h ${CS}/soundconfig.h
                        ${CS}/hostsettingsmenu.h ${CS}/forcefeedbackconfig.h
                        ${CS}/languageconfig.h)

SET(RS racescreens)
SET(RACESCREENS_SOURCES ${RS}/raceselectmenu.cpp
                        #${RS}/raceweekendmenu.cpp
                        ${RS}/racemanmenu.cpp ${RS}/fileselect.cpp
                        ${RS}/raceconfigstate.cpp
                        ${RS}/trackselect.cpp ${RS}/driverselect.cpp ${RS}/garagemenu.cpp
                        ${RS}/raceparamsmenu.cpp ${RS}/carsetupmenu.cpp
                        ${RS}/raceloadingmenu.cpp ${RS}/racerunningmenus.cpp
                        ${RS}/raceoptimizationmenu.cpp
                        ${RS}/racestopmenu.cpp ${RS}/racestartmenu.cpp ${RS}/racepitmenu.cpp
                        ${RS}/raceresultsmenus.cpp ${RS}/racenexteventmenu.cpp)
IF(OPTION_CLIENT_SERVER)
   SET(RACESCREENS_SOURCES ${RACESCREENS_SOURCES} ${RS}/csnetworkingmenu.cpp
                                                  ${RS}/csnetclientsettings.cpp
                                                  ${RS}/csnetserversettings.cpp)
ELSE()
   SET(RACESCREENS_SOURCES ${RACESCREENS_SOURCES} ${RS}/networkingmenu.cpp)
ENDIF(OPTION_CLIENT_SERVER)

SET(RACESCREENS_HEADERS ${RS}/racescreens.h ${RS}/garagemenu.h ${RS}/carsetupmenu.h) #${RS}/raceweekendscreens.h)
IF(OPTION_CLIENT_SERVER)
   SET(RACESCREENS_HEADERS ${RACESCREENS_HEADERS} ${RS}/csnetworking.h)
ENDIF(OPTION_CLIENT_SERVER)

SET(LEGACYMENU_SOURCES ${MAINSCREENS_SOURCES}
                       ${CONFSCREENS_SOURCES}
                       ${RACESCREENS_SOURCES}
                       legacymenu.cpp)
SET(LEGACYMENU_HEADERS ${MAINSCREENS_HEADERS}
                       ${CONFSCREENS_HEADERS}
                       ${RACESCREENS_HEADERS}
                       legacymenu.h)

INCLUDE_DIRECTORIES(. ${MS} ${CS} ${RS})
ADD_INTERFACE_INCLUDEDIR()

IF(OPTION_CLIENT_SERVER)
   SET(NETWORKING_INC csnetworking)
   SET(NETWORKING_LIB csnetworking)
ELSE(OPTION_CLIENT_SERVER)
   SET(NETWORKING_INC networking)
   SET(NETWORKING_LIB networking)
ENDIF(OPTION_CLIENT_SERVER)

ADD_SDLIB_INCLUDEDIR(math portability tgf tgfclient tgfdata ${NETWORKING_INC})

ADD_PLIB_INCLUDEDIR()
ADD_SDL2_INCLUDEDIR()
ADD_ENET_INCLUDEDIR()
ADD_CURL_INCLUDEDIR()
ADD_CJSON_INCLUDEDIR()
ADD_ZLIB_INCLUDEDIR()
ADD_MINIZIP_INCLUDEDIR()

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DLEGACYMENU_DLL)
ENDIF(WIN32)

IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG
        "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmtd.lib")
    SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE
        "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)

# Disable developer warning
IF(COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

# Note: Headers needed for having them available in IDEs.
SD_ADD_LIBRARY(legacymenu MODULE ${LEGACYMENU_SOURCES} ${LEGACYMENU_HEADERS})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose)
#SET_TARGET_PROPERTIES(legacymenu PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

find_package(rhash REQUIRED)

ADD_SDLIB_LIBRARY(legacymenu
    portability
    tgf
    tgfclient
    tgfdata
    ${NETWORKING_LIB}
    minizip::minizip
    rhash::rhash
)

target_compile_features(legacymenu PRIVATE cxx_std_11)
set_target_properties(legacymenu PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF)

ADD_PLIB_LIBRARY(legacymenu)
ADD_SDL2_LIBRARY(legacymenu)
ADD_ENET_LIBRARY(legacymenu)
ADD_CURL_LIBRARY(legacymenu)
ADD_CJSON_LIBRARY(legacymenu)
ADD_ZLIB_LIBRARY(legacymenu)
ADD_MINIZIP_LIBRARY(legacymenu)

SD_INSTALL_FILES(LIB modules/userinterface TARGETS legacymenu)
