INCLUDE(../../../cmake/macros.cmake)

#PROJECT(tgfclient)

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(tgf math portability)

ADD_SDL2_INCLUDEDIR()
ADD_SDL2_MIXER_INCLUDEDIR()

ADD_JPEG_INCLUDEDIR()
ADD_PLIB_INCLUDEDIR()
ADD_CURL_INCLUDEDIR()

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

SET(_SOURCES control.cpp glfeatures.cpp guibutton.cpp guifont.cpp
            guiimage.cpp guimenu.cpp guiscrollbar.cpp guitexture.cpp
            tgfclient.cpp gui.cpp guiedit.cpp guihelp.cpp
            guilabel.cpp guiobject.cpp guiscrollist.cpp
            guicombobox.cpp guicheckbox.cpp guiprogresbar.cpp
            guiscreen.cpp guieventloop.cpp guiapplication.cpp
            musicplayer.cpp musicplayer.h
            sdl2musicplayer.cpp sdl2musicplayer.h
            glfeatures.h gui.h guiscreen.h guimenu.h tgfclient.h guifont.h
            forcefeedback.cpp forcefeedback.h
            guimenusfx.cpp guimenusfx.h
            webserver.cpp webserver.h
            tgfclient_api.h translation.h)

SET(_OTHER_SOURCES guimenutest.xml)

#disable developer warning
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(WIN32)
    # DLL export stuff under Windows (to avoid .def file)
    ADD_DEFINITIONS(-DTGFCLIENT_DLL)
ENDIF(WIN32)

IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG
        "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib")
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE
        "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)

# Note: Other sources needed for having them available in IDEs.
SD_ADD_LIBRARY(tgfclient SHARED ${_SOURCES} ${_OTHER_SOURCES})

target_include_directories(tgfclient PUBLIC ${CMAKE_CURRENT_LIST_DIR}
    PRIVATE ${SDL2_TTF_INCLUDE_DIRS})

# Might not work with GCC 4.5 or + (non-robot modules crash at 1st reload = after 1 dlclose)
#SET_TARGET_PROPERTIES(tgfclient PROPERTIES VERSION ${VERSION} SOVERSION 0.0.0)

ADD_PLIB_LIBRARY(tgfclient ul sg)

#ADD_SDLIB_LIBRARY(tgfclient portability tgf networking)
ADD_SDLIB_LIBRARY(tgfclient portability tgf)

ADD_PNG_LIBRARY(tgfclient)
ADD_JPEG_LIBRARY(tgfclient)
target_link_libraries(tgfclient CURL::libcurl ${SDL2_TTF_LIBRARY})

IF(OPENGL_FOUND)
    TARGET_LINK_LIBRARIES(tgfclient ${OPENGL_LIBRARY})
ENDIF(OPENGL_FOUND)

ADD_SDL2_LIBRARY(tgfclient)
ADD_SDL2_MIXER_LIBRARY(tgfclient)


IF(WIN32)
#joystick library
    TARGET_LINK_LIBRARIES(tgfclient winmm)
ENDIF(WIN32)

IF(UNIX AND NOT HAIKU)
    ADD_X11_LIBRARY(tgfclient)
    IF(OPTION_XRANDR)
        ADD_XRANDR_LIBRARY(tgfclient)
    ENDIF(OPTION_XRANDR)
ENDIF(UNIX AND NOT HAIKU)

IF(UNIX)
    SD_INSTALL_FILES(LIB lib TARGETS tgfclient)
ELSE(UNIX)
    SD_INSTALL_FILES(BIN TARGETS tgfclient)
ENDIF(UNIX)

SD_INSTALL_FILES(INCLUDE FILES glfeatures.h guiscreen.h tgfclient.h)

IF(NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
   # To avoid 'undefined reference to glActiveTextureARB [--no-allow-shlib-undefined]'
   IF(UNIX AND NOT APPLE)
	   IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		    target_link_options(tgfclient PRIVATE -Wl,--allow-shlib-undefined)
	   ENDIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
   ENDIF(UNIX AND NOT APPLE)
ENDIF(NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
