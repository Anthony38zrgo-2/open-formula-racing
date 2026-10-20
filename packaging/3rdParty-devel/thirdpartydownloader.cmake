#
#==============================================================================
#
#    file                 : thirdpartydownloader.cmake
#    created              : June 22 2020
#    copyright            : (C) 2020 Joe Thompson
#    email                : beaglejoe@users.sourceforge.net
#
#==============================================================================
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#==============================================================================
#
# (hashtag) starts a comment
#cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

# Make DOWNLOAD_DIRECTORY absolute before use
set(DOWNLOAD_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/downloads" CACHE PATH "Location for downloaded files")
set(CACHED_URL "" CACHE STRING "Preferred url form which to download")
#message(STATUS "DOWNLOAD_DIRECTORY : " ${DOWNLOAD_DIRECTORY})
#message(STATUS "CACHED_URL = " ${CACHED_URL})
#message(STATUS "CMAKE_CURRENT_BINARY_DIR : " ${CMAKE_CURRENT_BINARY_DIR})
#message(STATUS "CMAKE_SOURCE_DIR : " ${CMAKE_SOURCE_DIR})
#message(STATUS "CMAKE_CURRENT_LIST_DIR : " ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_LIST_DIR}")

#include(${CMAKE_CURRENT_LIST_DIR}/3rdPartyDefinitions.cmake)
include(thirdpartydefinitions)

set(IS_FILE_URL False)
if(NOT CACHED_URL STREQUAL "")
   string(SUBSTRING ${CACHED_URL} 0 5 _WORSTR)
   string(TOLOWER ${_WORSTR} _WORSTR)
   if(_WORSTR STREQUAL "file:")
      set(IS_FILE_URL True)
   endif()

   if(IS_FILE_URL)
      set(SDL2_URL "${CACHED_URL}/${SDL2_FILE}")
      set(SDL2_MIXER_URL "${CACHED_URL}/${SDL2_MIXER_FILE}")
      set(SDL2_LEGACY_URL "${CACHED_URL}/${SDL2_LEGACY_FILE}")
      set(OPENAL_URL "${CACHED_URL}/${OPENAL_FILE}")
      set(OPENAL_C11_URL "${CACHED_URL}/${OPENAL_C11_FILE}")
      set(OPENAL_LEGACY_URL "${CACHED_URL}/${OPENAL_LEGACY_FILE}")
      set(PLIB_HEAD_URL "${CACHED_URL}/${PLIB_HEAD_FILE}")
      set(PLIB_URL "${CACHED_URL}/${PLIB_FILE}")
      set(JPEG_URL "${CACHED_URL}/${JPEG_FILE}")
      set(ENET_URL "${CACHED_URL}/${ENET_FILE}")
      set(OGG_URL "${CACHED_URL}/${OGG_FILE}")
      set(VORBIS_URL "${CACHED_URL}/${VORBIS_FILE}")
      set(EXPAT_URL "${CACHED_URL}/${EXPAT_FILE}")
      set(EXPAT_LEGACY_URL "${CACHED_URL}/${EXPAT_LEGACY_FILE}")
      set(ZLIB_URL "${CACHED_URL}/${ZLIB_FILE}")
      set(PNG_URL "${CACHED_URL}/${PNG_FILE}")
      set(FREETYPE_URL "${CACHED_URL}/${FREETYPE_FILE}")
      set(FREETYPE_LEGACY_URL "${CACHED_URL}/${FREETYPE_LEGACY_FILE}")
      set(CURL_URL "${CACHED_URL}/${CURL_FILE}")
      set(OSG_URL "${CACHED_URL}/${OSG_FILE}")
      set(SQLITE3_URL "${CACHED_URL}/${SQLITE3_FILE}")
      set(GLM_URL "${CACHED_URL}/${GLM_FILE}")
      set(TINYGLTF_URL "${CACHED_URL}/${TINYGLTF_FILE}")
      set(MINIZIP_URL "${CACHED_URL}/${MINIZIP_FILE}")
      set(CJSON_URL "${CACHED_URL}/${CJSON_FILE}")
   else()
      set(SDL2_URL "${CACHED_URL}/${SDL2_FILE} ${SDL2_URL}")
      set(SDL2_LEGACY_URL "${CACHED_URL}/${SDL2_LEGACY_FILE} ${SDL2_LEGACY_URL}")
      set(SDL2_MIXER_URL "${CACHED_URL}/${SDL2_MIXER_FILE} ${SDL2_MIXER_URL}")
      set(OPENAL_URL "${CACHED_URL}/${OPENAL_FILE} ${OPENAL_URL}")
      set(OPENAL_C11_URL "${CACHED_URL}/${OPENAL_C11_FILE} ${OPENAL_C11_URL}")
      set(OPENAL_LEGACY_URL "${CACHED_URL}/${OPENAL_LEGACY_FILE} ${OPENAL_LEGACY_URL}")
      set(PLIB_HEAD_URL "${CACHED_URL}/${PLIB_HEAD_FILE} ${PLIB_HEAD_URL}")
      set(PLIB_URL "${CACHED_URL}/${PLIB_FILE} ${PLIB_URL}")
      set(JPEG_URL "${CACHED_URL}/${JPEG_FILE} ${JPEG_URL}")
      set(ENET_URL "${CACHED_URL}/${ENET_FILE} ${ENET_URL}")
      set(OGG_URL "${CACHED_URL}/${OGG_FILE} ${OGG_URL}")
      set(VORBIS_URL "${CACHED_URL}/${VORBIS_FILE} ${VORBIS_URL}")
      set(EXPAT_URL "${CACHED_URL}/${EXPAT_FILE} ${EXPAT_URL}")
      set(EXPAT_LEGACY_URL "${CACHED_URL}/${EXPAT_LEGACY_FILE} ${EXPAT_LEGACY_URL}")
      set(ZLIB_URL "${CACHED_URL}/${ZLIB_FILE} ${ZLIB_URL}")
      set(PNG_URL "${CACHED_URL}/${PNG_FILE} ${PNG_URL}")
      set(FREETYPE_URL "${CACHED_URL}/${FREETYPE_FILE} ${FREETYPE_URL}")
      set(FREETYPE_LEGACY_URL "${CACHED_URL}/${FREETYPE_LEGACY_FILE} ${FREETYPE_LEGACY_URL}")
      set(CURL_URL "${CACHED_URL}/${CURL_FILE} ${CURL_URL}")
      set(OSG_URL "${CACHED_URL}/${OSG_FILE} ${OSG_URL}")
      set(SQLITE3_URL "${CACHED_URL}/${SQLITE3_FILE} ${SQLITE3_URL}")
      set(GLM_URL "${CACHED_URL}/${GLM_FILE} ${GLM_URL}")
      set(TINYGLTF_URL "${CACHED_URL}/${TINYGLTF_FILE} ${TINYGLTF_URL}")
      set(MINIZIP_URL "${CACHED_URL}/${MINIZIP_FILE} ${MINIZIP_URL}")
      set(CJSON_URL "${CACHED_URL}/${CJSON_FILE} ${CJSON_URL}")
   endif()
endif()

################################################
function(SD_DownloadIfNeeded FILE_DESTINATION SRC_URL)
   if(EXISTS ${FILE_DESTINATION})
      if(NOT ${CMAKE_VERSION} VERSION_LESS "3.14.0")
         file(SIZE ${FILE_DESTINATION} _filesize)
          message(STATUS ${FILE_DESTINATION} " Exists and size = " ${_filesize})
         if(_filesize EQUAL 0)
            file(DOWNLOAD ${SRC_URL} ${FILE_DESTINATION} SHOW_PROGRESS STATUS DL_STATUS LOG DL_LOG)
         endif()
      endif()
   else()
      file(DOWNLOAD ${SRC_URL} ${FILE_DESTINATION} SHOW_PROGRESS STATUS DL_STATUS LOG DL_LOG)
   endif()
   if(DL_STATUS)
      list(GET DL_STATUS 0 STAT_NUM)
      list(GET DL_STATUS 1 STAT_STRING)
      if(NOT STAT_NUM EQUAL 0)
         message("Failed Download of ${SRC_URL}")
         message("DL_STATUS number = " ${STAT_NUM})
         message("DL_STATUS = " ${STAT_STRING})
         message("DL_LOG = ${DL_LOG}")
         message("")
         message(SEND_ERROR "Failed Download of ${SRC_URL}")
      endif()
   endif()
endfunction()

################################################

SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${SDL2_FILE} ${SDL2_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${SDL2_LEGACY_FILE} ${SDL2_LEGACY_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${SDL2_MIXER_FILE} ${SDL2_MIXER_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${OPENAL_LEGACY_FILE} ${OPENAL_LEGACY_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${OPENAL_C11_FILE} ${OPENAL_C11_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${OPENAL_FILE} ${OPENAL_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${PLIB_HEAD_FILE} ${PLIB_HEAD_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${PLIB_FILE} ${PLIB_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${JPEG_FILE} ${JPEG_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${ENET_FILE} ${ENET_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${OGG_FILE} ${OGG_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${VORBIS_FILE} ${VORBIS_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${EXPAT_FILE} ${EXPAT_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${EXPAT_LEGACY_FILE} ${EXPAT_LEGACY_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${ZLIB_FILE} ${ZLIB_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${PNG_FILE} ${PNG_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${FREETYPE_FILE} ${FREETYPE_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${FREETYPE_LEGACY_FILE} ${FREETYPE_LEGACY_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${CURL_FILE} ${CURL_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${OSG_FILE} ${OSG_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${SQLITE3_FILE} ${SQLITE3_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${GLM_FILE} ${GLM_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${TINYGLTF_FILE} ${TINYGLTF_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${MINIZIP_FILE} ${MINIZIP_URL})
SD_DownloadIfNeeded(${DOWNLOAD_DIRECTORY}/${CJSON_FILE} ${CJSON_URL})


message(STATUS "DOWNLOAD_DIRECTORY = " ${DOWNLOAD_DIRECTORY})
file(GLOB FLIST RELATIVE ${DOWNLOAD_DIRECTORY} ${DOWNLOAD_DIRECTORY}/*.*)

file(WRITE ${DOWNLOAD_DIRECTORY}/sha256.txt "")
foreach(_file ${FLIST})
   file(SHA256 ${DOWNLOAD_DIRECTORY}/${_file} _HASH)
   #message(STATUS ${_HASH} " : " ${_file})
   set(_ENTRY "${_HASH} : ${_file}\n")
   file(APPEND ${DOWNLOAD_DIRECTORY}/sha256.txt ${_ENTRY})
endforeach()
