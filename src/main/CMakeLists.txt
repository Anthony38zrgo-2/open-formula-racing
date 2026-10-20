INCLUDE(../../cmake/macros.cmake)

PROJECT(speed-dreams-2)

SET(_TARGET_NAME speed-dreams-2)

SET(_SOURCES main.cpp)
IF(MSVC)
	SET(_SOURCES ${_SOURCES} main.rc resource.h ../../data/data/icons/icon.ico)
ENDIF(MSVC)

IF(NOT SD_ASSUME_DATADIR)
	configure_file(${CMAKE_SOURCE_DIR}/cmake/${PROJECT_NAME}.rc.in.cmake
		${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc)
	SET(_SOURCES ${_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.rc)
ENDIF()

ADD_INTERFACE_INCLUDEDIR()
ADD_SDLIB_INCLUDEDIR(tgf tgfclient tgfdata math portability)

ADD_SDL2_INCLUDEDIR()

ADD_PLIB_INCLUDEDIR()

# Disable developer warning
IF(COMMAND CMAKE_POLICY)
	CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

IF(MSVC)
    # Ignore some run-time libs to avoid link time warnings and sometimes even crashes.
    SET(CMAKE_EXE_LINKER_FLAGS_DEBUG
        "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:libcmt.lib")
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libcmt.lib")
ENDIF(MSVC)

SD_ADD_EXECUTABLE(${_TARGET_NAME} ${_SOURCES})

ADD_SDLIB_LIBRARY(${_TARGET_NAME} portability tgf tgfclient tgfdata)

# Needed when using the "MinGW Makefiles" CMake generator (but not with the "MSYS Makefiles" one !)
IF(MINGW)
	TARGET_LINK_LIBRARIES(${_TARGET_NAME} ${MINGW32_LIBRARY})
ENDIF(MINGW)

IF(UNIX)
    SD_INSTALL_FILES(MAN man6 PREFIX ${CMAKE_SOURCE_DIR}/doc/man FILES ${_TARGET_NAME}.6)
    # Install a launcher for desktop environments.
    configure_file(
        ${CMAKE_SOURCE_DIR}/speed-dreams.desktop.in
        ${PROJECT_BINARY_DIR}/speed-dreams.desktop @ONLY)
    install(
        FILES ${PROJECT_BINARY_DIR}/speed-dreams.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
ENDIF(UNIX)

SD_INSTALL_FILES(BIN TARGETS ${_TARGET_NAME})

IF((APPLE) AND ("${CMAKE_INSTALL_PREFIX}" MATCHES "\\.app$"))
   INSTALL(FILES ../../packaging/OSX/Info.plist DESTINATION ./Contents)
   INSTALL(FILES ../../packaging/OSX/sd-icon-mac-256.icns DESTINATION ./Contents/Resources)
ENDIF()

# Under Windows, install needed 3rd party DLLs close to Speed Dreams executable.
IF(WIN32 AND OPTION_CUSTOM_3RDPARTY)
	INCLUDE(customthirdparty)
	SD_INSTALL_CUSTOM_3RDPARTY(${_TARGET_NAME})
ENDIF(WIN32 AND OPTION_CUSTOM_3RDPARTY)

IF(NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
# To avoid 'undefined reference to glActiveTextureARB [--no-allow-shlib-undefined]'
   IF(UNIX AND NOT APPLE)
	   IF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		    target_link_options(${_TARGET_NAME} PRIVATE -Wl,--allow-shlib-undefined)
	   ENDIF(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	   target_link_options(${_TARGET_NAME} PRIVATE -Wl,-E)
   ENDIF(UNIX AND NOT APPLE)
ENDIF(NOT ${CMAKE_VERSION} VERSION_LESS "3.13")
