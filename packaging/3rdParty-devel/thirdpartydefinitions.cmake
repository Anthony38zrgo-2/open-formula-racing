#
#==============================================================================
#
#    file                 : thirdpartydefinitions.cmake
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
#
#           NOTICE
# When updating OpenSceneGraph, libPNG, or jpeg
# Check /cmake/customthirdparty.cmake
#  The macros
#     MACRO(SD_INSTALL_CUSTOM_3RDPARTY TARGET_NAME)
#     MACRO(_FIND_3RDPARTY_DEPENDENCIES ROOT_DIR)
#  may need to be updated for the new version


# SDL2
set(SDL2_VERSION 2.30.5)
set(SDL2_PROJECT SDL2-${SDL2_VERSION})
set(SDL2_FILE ${SDL2_PROJECT}.tar.gz)
set(SDL2_URL https://www.libsdl.org/release/${SDL2_FILE})
set(SDL2_HASH SHA256=f374f3fa29c37dfcc20822d4a7d7dc57e58924d1a5f2ad511bfab4c8193de63b)

set(SDL2_LEGACY_VERSION 2.24.2)
set(SDL2_LEGACY_PROJECT SDL2-${SDL2_LEGACY_VERSION})
set(SDL2_LEGACY_FILE ${SDL2_LEGACY_PROJECT}.tar.gz)
set(SDL2_LEGACY_URL https://www.libsdl.org/release/${SDL2_LEGACY_FILE})
set(SDL2_LEGACY_HASH SHA256=b35ef0a802b09d90ed3add0dcac0e95820804202914f5bb7b0feb710f1a1329f)

# SDL2_MIXER
set(SDL2_MIXER_VERSION 2.8.0)
set(SDL2_MIXER_PROJECT SDL2_mixer-${SDL2_MIXER_VERSION})
set(SDL2_MIXER_FILE ${SDL2_MIXER_PROJECT}.tar.gz)
#set(SDL2_MIXER_URL https://www.libsdl.org/projects/SDL_mixer/release/${SDL2_MIXER_FILE})

set(SDL2_MIXER_URL https://github.com/libsdl-org/SDL_mixer/releases/download/release-${SDL2_MIXER_VERSION}/${SDL2_MIXER_FILE})
#https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.6.1/SDL2_mixer-2.6.1.tar.gz
set(SDL2_MIXER_HASH SHA256=1cfb34c87b26dbdbc7afd68c4f545c0116ab5f90bbfecc5aebe2a9cb4bb31549)

# SDL2_TTF
set(SDL2_TTF_VERSION 2.22.0)
set(SDL2_TTF_PROJECT SDL2_ttf-${SDL2_TTF_VERSION})
set(SDL2_TTF_FILE release-${SDL2_TTF_VERSION}.tar.gz)
set(SDL2_TTF_URL https://github.com/libsdl-org/SDL_ttf/archive/refs/tags/${SDL2_TTF_FILE})
set(SDL2_TTF_HASH SHA256=2275d0ddfffa53f0efa628bc1621f662dacbd42467b5a44db99e38255fbb575a)

# OpenAL-soft
set(OPENAL_PROJECT_VERSION 1.23.1)
set(OPENAL_PROJECT openal-soft-${OPENAL_PROJECT_VERSION})
set(OPENAL_FILE ${OPENAL_PROJECT}.tar.gz)
set(OPENAL_URL https://github.com/kcat/openal-soft/archive/refs/tags/${OPENAL_PROJECT_VERSION}.tar.gz)
set(OPENAL_HASH SHA256=dfddf3a1f61059853c625b7bb03de8433b455f2f79f89548cbcbd5edca3d4a4a)

# https://github.com/kcat/openal-soft/releases/tag/1.23.1
# https://github.com/kcat/openal-soft/archive/refs/tags/1.23.1.tar.gz

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    message(WARNING "openal-soft versions newer than 1.19.1 need Visual Studio 2017 and CMAKE_SYSTEM_VERSION >= 10")
endif()
set(OPENAL_C11_VERSION 1.19.1)
set(OPENAL_C11_PROJECT openal-soft-${OPENAL_C11_VERSION})
set(OPENAL_C11_FILE ${OPENAL_C11_PROJECT}.tar.gz)
set(OPENAL_C11_URL https://github.com/kcat/openal-soft/archive/refs/tags/${OPENAL_C11_FILE})
set(OPENAL_C11_HASH SHA256=9f3536ab2bb7781dbafabc6a61e0b34b17edd16bd6c2eaf2ae71bc63078f98c7)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    message(STATUS "openal-soft versions newer than 1.18.2 need Visual Studio 2015 or newer")
    message(STATUS "so for older versions of Visual Studio, also download this version")
endif()
set(OPENAL_LEGACY_VERSION 1.18.2)
set(OPENAL_LEGACY_PROJECT openal-soft-${OPENAL_LEGACY_VERSION})
set(OPENAL_LEGACY_FILE ${OPENAL_LEGACY_PROJECT}.tar.gz)
set(OPENAL_LEGACY_URL https://github.com/kcat/openal-soft/archive/refs/tags/${OPENAL_LEGACY_FILE})
set(OPENAL_LEGACY_HASH SHA256=a598241d1af2e90c25a1b91da4c9ddc0e7cb6a4b5f1477fc680d139c57cd38cc)

# PLIB
set(PLIB_TAG v1.8.5-sd3)
set(PLIB_PROJECT plib-${PLIB_TAG})
set(PLIB_URL https://forge.a-lec.org/speed-dreams/plib/archive/${PLIB_TAG}.tar.gz)
set(PLIB_HASH SHA256=2ac882854467aea9ed077f2c522366bcba50dda2c92b79064230d697a514b37b)

# jpeg
set(JPEG_VERSION 9f)
set(JPEG_PROJECT jpeg-${JPEG_VERSION})
set(JPEG_FILE jpegsrc.v${JPEG_VERSION}.tar.gz)
set(JPEG_URL https://ijg.org/files/${JPEG_FILE})
set(JPEG_HASH SHA256=04705c110cb2469caa79fb71fba3d7bf834914706e9641a4589485c1f832565b)

# enet
set(ENET_VERSION 1.3.18)
set(ENET_PROJECT enet-${ENET_VERSION})
set(ENET_FILE ${ENET_PROJECT}.tar.gz)
set(ENET_URL https://github.com/lsalzman/enet/archive/refs/tags/v${ENET_VERSION}.tar.gz)
set(ENET_HASH SHA256=28603c895f9ed24a846478180ee72c7376b39b4bb1287b73877e5eae7d96b0dd)

# ogg
set(OGG_VERSION 1.3.5)
set(OGG_PROJECT ogg-${OGG_VERSION})
set(OGG_FILE lib${OGG_PROJECT}.tar.gz)
set(OGG_URL http://downloads.xiph.org/releases/ogg/${OGG_FILE})
set(OGG_HASH SHA256=0eb4b4b9420a0f51db142ba3f9c64b333f826532dc0f48c6410ae51f4799b664)

# vorbis
set(VORBIS_VERSION 1.3.7)
set(VORBIS_PROJECT vorbis-${VORBIS_VERSION})
set(VORBIS_FILE lib${VORBIS_PROJECT}.tar.gz)
set(VORBIS_URL http://downloads.xiph.org/releases/vorbis/${VORBIS_FILE})
set(VORBIS_HASH SHA256=0e982409a9c3fc82ee06e08205b1355e5c6aa4c36bca58146ef399621b0ce5ab)

# expat
set(EXPAT_VERSION 2.6.2)
string(REPLACE "." "_" EXPAT_TAG ${EXPAT_VERSION})
set(EXPAT_PROJECT expat-${EXPAT_VERSION})
set(EXPAT_FILE ${EXPAT_PROJECT}.tar.bz2)
set(EXPAT_URL https://github.com/libexpat/libexpat/releases/download/R_${EXPAT_TAG}/${EXPAT_FILE})
set(EXPAT_HASH SHA256=9c7c1b5dcbc3c237c500a8fb1493e14d9582146dd9b42aa8d3ffb856a3b927e0)

set(EXPAT_LEGACY_VERSION 2.2.10)
string(REPLACE "." "_" EXPAT_LEGACY_TAG ${EXPAT_LEGACY_VERSION})
set(EXPAT_LEGACY_PROJECT expat-${EXPAT_LEGACY_VERSION})
set(EXPAT_LEGACY_FILE ${EXPAT_LEGACY_PROJECT}.tar.bz2)
set(EXPAT_LEGACY_URL https://github.com/libexpat/libexpat/releases/download/R_${EXPAT_LEGACY_TAG}/${EXPAT_LEGACY_FILE})
set(EXPAT_LEGACY_HASH SHA256=b2c160f1b60e92da69de8e12333096aeb0c3bf692d41c60794de278af72135a5)

# zlib
message(STATUS "Note special path handling (version in path)")
set(ZLIB_VERSION 1.3.1)
set(ZLIB_PROJECT zlib-${ZLIB_VERSION})
set(ZLIB_FILE ${ZLIB_PROJECT}.tar.gz)
#set(ZLIB_URL https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/{ZLIB_FILE}.tar.gz)
#set(ZLIB_HASH SHA256=e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855)
set(ZLIB_URL https://zlib.net/${ZLIB_FILE})
set(ZLIB_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23)

# libpng
message(STATUS "Note special path handling (version in path) AND hard-coded 'libpng16'")
set(PNG_VERSION 1.6.43)
set(PNG_PROJECT libpng-${PNG_VERSION})
set(PNG_FILE ${PNG_PROJECT}.tar.gz)
set(PNG_URL https://sourceforge.net/projects/libpng/files/libpng16/${PNG_VERSION}/${PNG_FILE}/download)
set(PNG_HASH SHA256=e804e465d4b109b5ad285a8fb71f0dd3f74f0068f91ce3cdfde618180c174925)

# freetype
message(STATUS "Note special path handling (version in path) AND hard-coded 'freetype2'")
set(FREETYPE_VERSION 2.13.2)
set(FREETYPE_PROJECT freetype-${FREETYPE_VERSION})
string(REPLACE "." "" FREETYPE_TAG ${FREETYPE_VERSION})
set(FREETYPE_FILE ft${FREETYPE_TAG}.zip)
set(FREETYPE_URL https://sourceforge.net/projects/freetype/files/freetype2/${FREETYPE_VERSION}/${FREETYPE_FILE}/download)
set(FREETYPE_HASH SHA256=b7e5b03d2e890c4a881e9fab5870463a37fc9cb934c886b9aab2f6fd637ae783)

message(STATUS "Note special path handling (version in path) AND hard-coded 'freetype2'")
set(FREETYPE_LEGACY_VERSION 2.10.4)
set(FREETYPE_LEGACY_PROJECT freetype-${FREETYPE_LEGACY_VERSION})
set(FREETYPE_LEGACY_FILE ft2104.zip)
set(FREETYPE_LEGACY_URL https://sourceforge.net/projects/freetype/files/freetype2/${FREETYPE_LEGACY_VERSION}/${FREETYPE_LEGACY_FILE}/download)
set(FREETYPE_LEGACY_HASH SHA256=5c78216d6c5860ef694fde1418d20d69d0ac83ab346c21eb311bd45709e0d93a)

# curl
set(CURL_VERSION 8.8.0)
string(REPLACE "." "_" CURL_TAG ${CURL_VERSION})
set(CURL_PROJECT curl-${CURL_VERSION})
set(CURL_FILE ${CURL_PROJECT}.tar.bz2)
#set(CURL_URL https://curl.se/download/${CURL_FILE})
set(CURL_URL https://github.com/curl/curl/releases/download/curl-${CURL_TAG}/${CURL_FILE})
set(CURL_HASH SHA256=40d3792d38cfa244d8f692974a567e9a5f3387c547579f1124e95ea2a1020d0d)

# osg
set(OSG_VERSION 3.6.5)
set(OSG_PROJECT OpenSceneGraph-${OSG_VERSION})
set(OSG_FILE ${OSG_PROJECT}.zip)
set(OSG_URL https://github.com/openscenegraph/OpenSceneGraph/archive/${OSG_FILE})
set(OSG_HASH SHA256=0e9e3e4cc6f463f21a901934a95e9264b231a1d5db90f72dcb4b8cc94b0d1b3b)

# sqlite3
message(STATUS "Note the YEAR in the path AND hard-coded filename")
set(SQLITE3_VERSION 3.46.0)
set(SQLITE3_PROJECT sqlite3-${SQLITE3_VERSION})
set(SQLITE3_FILE sqlite-amalgamation-3460000.zip)
set(SQLITE3_URL https://www.sqlite.org/2024/${SQLITE3_FILE})
set(SQLITE3_HASH SHA256=712a7d09d2a22652fb06a49af516e051979a3984adb067da86760e60ed51a7f5)

# GLM
set(GLM_VERSION 1.0.1)
set(GLM_PROJECT glm-${GLM_VERSION})
set(GLM_FILE ${GLM_VERSION}.tar.gz)
set(GLM_URL https://github.com/g-truc/glm/archive/refs/tags/${GLM_VERSION}.tar.gz)
set(GLM_HASH SHA256=9f3174561fd26904b23f0db5e560971cbf9b3cbda0b280f04d5c379d03bf234c)

# TinyGLTF
set(TINYGLTF_VERSION 2.8.22)
set(TINYGLTF_PROJECT TinyGLTF-${TINYGLTF_VERSION})
set(TINYGLTF_FILE ${TINYGLTF_PROJECT}.tar.gz)
set(TINYGLTF_URL https://github.com/syoyo/tinygltf/archive/refs/tags/v${TINYGLTF_VERSION}.tar.gz)
set(TINYGLTF_HASH SHA256=97c3eb1080c1657cd749d0b49af189c6a867d5af30689c48d5e19521e7b5a070)

# minizip
set(MINIZIP_VERSION 1.3.1)
set(MINIZIP_PROJECT minizip-${MINIZIP_VERSION})
set(MINIZIP_URL https://github.com/F2I-Consulting/Minizip/archive/refs/tags/v1.3_cmake0.1.tar.gz)
set(MINIZIP_HASH SHA256=4ab1ffdc954e10faf2e756bd26e52ac4c339389159725dacb3d9104560f7f1a3)

# cjson
set(CJSON_VERSION 1.7.18)
set(CJSON_PROJECT cjson-${CJSON_VERSION})
set(CJSON_FILE ${CJSON_PROJECT}.tar.gz)
set(CJSON_URL https://github.com/DaveGamble/cJSON/archive/refs/tags/v${CJSON_VERSION}.tar.gz)
set(CJSON_HASH SHA256=3aa806844a03442c00769b83e99970be70fbef03735ff898f4811dd03b9f5ee5)

# rhash
set(RHASH_VERSION 1.4.4)
set(RHASH_PROJECT rhash-${RHASH_VERSION})
set(RHASH_FILE ${RHASH_PROJECT}-src.tar.gz)
set(RHASH_URL https://deac-riga.dl.sourceforge.net/project/rhash/rhash/${RHASH_VERSION}/${RHASH_FILE}?viasf=1)
set(RHASH_HASH SHA256=8e7d1a8ccac0143c8fe9b68ebac67d485df119ea17a613f4038cda52f84ef52a)
