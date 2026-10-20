#  Speed Dreams, a free and open source motorsport simulator.
#  Copyright (C) 2019 Joe Thompson, 2025 Xavier Del Campo Romero
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.

set(CPACK_NSIS_INSTALLED_ICON_NAME ${PROJECT_NAME})
set(CPACK_NSIS_MUI_FINISHPAGE_RUN ${PROJECT_NAME})
set(CPACK_NSIS_DISPLAY_NAME "Speed Dreams ${CPACK_PACKAGE_VERSION}")
set(CPACK_NSIS_URL_INFO_ABOUT "${CMAKE_PROJECT_HOMEPAGE_URL}")
set(CPACK_NSIS_HELP_LINK "https://forge.a-lec.org/speed-dreams/")
set(CPACK_PACKAGE_EXECUTABLES
   ${PROJECT_NAME} "${CPACK_NSIS_DISPLAY_NAME}"
)
set(CPACK_CREATE_DESKTOP_LINKS ${PROJECT_NAME})
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL True)

# Extra shortcuts to add in the start menu (a list of pairs : URL, Menu label).
set(CPACK_NSIS_MENU_LINKS
   "${CMAKE_PROJECT_HOMEPAGE_URL}" "Project website"
   "https://forge.a-lec.org/speed-dreams/" "Project source repositories"
   "/doc/userman/how_to_drive.html" "User manual")

if(NOT SD_ASSUME_DATADIR)
   set(CPACK_NSIS_MUI_ICON "${SD_DATADIR_ABS}/data/icons/icon.ico")
   set(CPACK_NSIS_MUI_UNIICON "${SD_DATADIR_ABS}/data/icons/icon.ico")
   set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${SD_DATADIR_ABS}/data/img/header-vert.bmp")
   set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${SD_DATADIR_ABS}/data/img/header-vert.bmp")

   if(NOT ${CMAKE_VERSION} VERSION_LESS "3.17")
      set(CPACK_NSIS_MUI_HEADERIMAGE "${SD_DATADIR_ABS}/data/img/header.bmp")
   endif(NOT ${CMAKE_VERSION} VERSION_LESS "3.17")
endif()
