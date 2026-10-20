############################################################################
#
#   file        : FindCustom3rdParty.cmake
#   copyright   : (C) 2009 by Brian Gavin, 2012 Jean-Philippe Meuret
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

# @file     Custom 3rdParty location handling for some Windows builds
#           (standard CMake Find<package> macros can't find it, or don't do
#            it the way we need, so we needed another solution).
#           Heavily based on OpenScenGraph cmake scripts.
# @author   Brian Gavin, Jean-Philippe Meuret

################################################################################################
# Under Windows, install needed 3rd party DLLs close to Speed Dreams executable
# (but stay compatible with the old 2.0.0 3rd party package which had less DLLs inside)

# Find the full path-name of the 3rd party DLL corresponding to the given 3rd party link library
#
# Parameters :
# * LIB_PATH_NAMES : The link library (or list of link libraries) path-name.
# * LIB_NAME_HINTS : Hints for retrieving in LIB_PATH_NAMES the only lib we are taking care of,
#                    and for retrieving on disk the corresponding DLL.
# * DLL_NAME_PREFIXES : Possible prefixes (to the lib name hint) for retrieving the DLL.
#                       Note: the empty "" prefix is always tried at the end.
#                       Ex: "lib;xx" for "lib" and "xx" prefixes.
# * DLL_PATHNAME_VAR : Name of the output variable for the retrieved DLL path-name.

MACRO(_FIND_3RDPARTY_DLL LIB_PATH_NAMES LIB_NAME_HINTS DLL_NAME_PREFIXES DLL_PATHNAME_VAR)

	FOREACH(_LIB_NAME_HINT ${LIB_NAME_HINTS})

		# Must handle the case of multiple libs listed in ${LIB_PATH_NAMES} :
		# Use LIB_NAME_HINTS to retrieve the one we are interested in here.
		SET(_LIB_PATHNAME ${LIB_PATH_NAMES})
		FOREACH(_LIB_PATHNAME_ ${LIB_PATH_NAMES})
			IF(${_LIB_PATHNAME_} MATCHES "${_LIB_NAME_HINT}\\.")
				SET(_LIB_PATHNAME ${_LIB_PATHNAME_})
				BREAK()
			ENDIF(${_LIB_PATHNAME_} MATCHES "${_LIB_NAME_HINT}\\.")
		ENDFOREACH(_LIB_PATHNAME_ ${LIB_PATH_NAMES})

		# Got the link library pathname : check if any corresponding DLL around (try all prefixes).
		# 1) Check the empty prefix
		#    (CMake ignores it when specified at the beginning of DLL_NAME_PREFIXES ... bull shit).
		GET_FILENAME_COMPONENT(_LIB_PATH "${_LIB_PATHNAME}" PATH)
		SET(${DLL_PATHNAME_VAR} "${_LIB_PATH}/../bin/${_LIB_NAME_HINT}${CMAKE_SHARED_LIBRARY_SUFFIX}")
		#MESSAGE(STATUS "Trying 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
		IF(EXISTS "${${DLL_PATHNAME_VAR}}")
			MESSAGE(STATUS "Will install 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
			BREAK() # First found is the one.
		ELSE(EXISTS "${${DLL_PATHNAME_VAR}}")
			UNSET(${DLL_PATHNAME_VAR})
		ENDIF(EXISTS "${${DLL_PATHNAME_VAR}}")

		# 2) Check other (specified) prefixes.
		FOREACH(_DLL_NAME_PREFIX ${DLL_NAME_PREFIXES})
			SET(${DLL_PATHNAME_VAR} "${_LIB_PATH}/../bin/${_DLL_NAME_PREFIX}${_LIB_NAME_HINT}${CMAKE_SHARED_LIBRARY_SUFFIX}")
			#MESSAGE(STATUS "Trying 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
			IF(EXISTS "${${DLL_PATHNAME_VAR}}")
				BREAK() # First found is the one.
			ELSE(EXISTS "${${DLL_PATHNAME_VAR}}")
				UNSET(${DLL_PATHNAME_VAR})
			ENDIF(EXISTS "${${DLL_PATHNAME_VAR}}")
		ENDFOREACH(_DLL_NAME_PREFIX ${DLL_NAME_PREFIXES})

		IF(EXISTS "${${DLL_PATHNAME_VAR}}")
			MESSAGE(STATUS "Will install 3rdParty DLL ${${DLL_PATHNAME_VAR}}")
			BREAK() # First found is the one.
		ELSE(EXISTS "${${DLL_PATHNAME_VAR}}")
			UNSET(${DLL_PATHNAME_VAR})
		ENDIF(EXISTS "${${DLL_PATHNAME_VAR}}")

	ENDFOREACH(_LIB_NAME_HINT ${LIB_NAME_HINTS})

	#IF(NOT EXISTS "${${DLL_PATHNAME_VAR}}")
	#	MESSAGE(STATUS "Could not find 3rdParty DLL for lib ${LIB_NAME_HINTS} (prefixes ${DLL_NAME_PREFIXES})")
	#ENDIF()

ENDMACRO(_FIND_3RDPARTY_DLL DLL_PATHNAME)

MACRO(SD_INSTALL_CUSTOM_3RDPARTY TARGET_NAME)

	# 1) Find 3rd party DLL files to install.
	SET(_THIRDPARTY_DLL_PATHNAMES)

	_FIND_3RDPARTY_DLL("${OPENAL_LIBRARY}" "OpenAL32" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	# Menu Music requires ogg, vorbis, and vorbisfile
	_FIND_3RDPARTY_DLL("${OGG_LIBRARY}" "ogg;libogg;libogg-0" "" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${VORBIS_LIBRARY}" "vorbis;libvorbis;libvorbis-0" "" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${VORBISFILE_LIBRARY}" "vorbisfile;libvorbisfile;libvorbisfile-3" "" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${SDL2_LIBRARY}" "SDL2" ";lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${SDL2_MIXER_LIBRARY}" "SDL2_mixer" ";lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${SDL2_TTF_LIBRARY}" "SDL2_ttf" ";lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	IF(OPTION_3RDPARTY_EXPAT)

		_FIND_3RDPARTY_DLL("${EXPAT_LIBRARY}" "expat;expat-1" "lib" _DLL_PATHNAME)
		LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	ENDIF(OPTION_3RDPARTY_EXPAT)

	IF(OPTION_OSGGRAPH)

		# DLLs whose libs we link with.
		SET(_OSG_DLLS_NAME_HINTS "OpenThreads;osgDB;osgFX;osgGA;osgParticle;osgShadow;osgViewer;osgUtil;osg;osgText")
		FOREACH(_LIB_NAME ${OPENSCENEGRAPH_LIBRARIES})
			FOREACH(_NAME_HINT ${_OSG_DLLS_NAME_HINTS})
				IF("${_LIB_NAME}" MATCHES "${_NAME_HINT}\\.")
					_FIND_3RDPARTY_DLL("${_LIB_NAME}" "${_NAME_HINT}" "lib;ot21-;ot20-;ot12-;osg161-;osg160-;osg158-" _DLL_PATHNAME)
					SET(_NAME_HINT_ "${_NAME_HINT}") # For later (see below DLLs we don't link with).
					SET(_LIB_NAME_ "${_LIB_NAME}") # For later (see below DLLs we don't link with).
					SET(_DLL_PATHNAME_ "${_DLL_PATHNAME}") # For later (see below plugins).
					BREAK()
				ENDIF()
			ENDFOREACH()
			LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")
		ENDFOREACH()

		# Plugins : Complete the list right below according to the actual needs.
		# TODO: Find a way to install them in the osgPlugins-xxx subdir (works as is, but ...)
		SET(_OSG_PLUGIN_NAME_HINTS "osgdb_ac;osgdb_dds;osgdb_glsl") # ';'-separated list
		LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_ive;osgdb_jpeg;osgdb_osg;osgdb_curl;osgdb_freetype")
		LIST(APPEND _OSG_PLUGIN_NAME_HINTS "osgdb_osga;osgdb_osgshadow;osgdb_osgtgz;osgdb_png;osgdb_rgb")

		GET_FILENAME_COMPONENT(_OSG_PLUGINS_DIR "${_DLL_PATHNAME_}" PATH)
		FILE(GLOB_RECURSE _OSG_PLUGIN_NAMES "${_OSG_PLUGINS_DIR}/*${CMAKE_SHARED_LIBRARY_SUFFIX}")
		FOREACH(_NAME_HINT ${_OSG_PLUGIN_NAME_HINTS})
			FOREACH(_PLUGIN_NAME ${_OSG_PLUGIN_NAMES})
				IF("${_PLUGIN_NAME}" MATCHES "osgPlugins.*/.*${_NAME_HINT}\\.")
					LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_PLUGIN_NAME}")
					MESSAGE(STATUS "Will install 3rdParty OSG plugin ${_PLUGIN_NAME}")
					BREAK()
				ENDIF()
			ENDFOREACH()
		ENDFOREACH()

	ENDIF(OPTION_OSGGRAPH)

	_FIND_3RDPARTY_DLL("${ZLIB_LIBRARY}" "zlib;zlib1" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PNG_LIBRARY}" "png;png16;png15" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${JPEG_LIBRARY}" "jpeg;jpeg-9;jpeg-8" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	# CMake might already define FindFreetype.cmake, but it is not required
	# since freetype should already install freetype-config.cmake inside the
	# 3rdParty directory.
	find_package(freetype CONFIG REQUIRED)
	get_target_property(freetype_lib freetype LOCATION)
	_FIND_3RDPARTY_DLL("${freetype_lib}" "freetype" ";lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	get_property(CURL_LIBRARY TARGET CURL::libcurl PROPERTY LOCATION)
	_FIND_3RDPARTY_DLL("${CURL_LIBRARY}" "curl" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${GLM_LIBRARY}" "glm" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${TINYGLTF_LIBRARY}" "tinygltf" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${CJSON_LIBRARY}" "cjson" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${minizip_LIBRARIES}" "minizip" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${rhash_LIBRARIES}" "rhash" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${ENET_LIBRARY}" "enet" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PLIB_UL_LIBRARY}" "plibul" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PLIB_JS_LIBRARY}" "plibjs" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PLIB_SG_LIBRARY}" "plibsg" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PLIB_SSG_LIBRARY}" "plibssg" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PLIB_SSGAUX_LIBRARY}" "plibssgaux" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	_FIND_3RDPARTY_DLL("${PLIB_SL_LIBRARY}" "plibsl" "lib" _DLL_PATHNAME)
	LIST(APPEND _THIRDPARTY_DLL_PATHNAMES "${_DLL_PATHNAME}")

	# 2) Copy found 3rd party DLL files to the bin folder (for running without installing).
	#MESSAGE(STATUS "3rdParty dependencies : Will install ${_THIRDPARTY_DLL_PATHNAMES}")
    SET(_NOINST_DIR "${CMAKE_BINARY_DIR}/${SD_BINDIR}")
    ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD
                       COMMAND ${CMAKE_COMMAND} -E make_directory "${_NOINST_DIR}"
                       VERBATIM)
    FOREACH(_DLL ${_THIRDPARTY_DLL_PATHNAMES})
      ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD
                         COMMAND ${CMAKE_COMMAND} -E echo Copying "${_DLL}" to "${_NOINST_DIR}"
                         COMMAND ${CMAKE_COMMAND} -E copy "${_DLL}" "${_NOINST_DIR}"
                         VERBATIM)
    ENDFOREACH()

	# 3) Install found 3rd party DLL files to the install folder.
	SD_INSTALL_FILES(BIN FILES ${_THIRDPARTY_DLL_PATHNAMES})

	# 4) Find Windows compilers run-time DLLs.
	IF(MSVC)

		# We do it ourselves, but use InstallRequiredSystemLibraries to figure out which ones.
		SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
		INCLUDE(InstallRequiredSystemLibraries)
		SET(_COMPILER_DLL_PATHNAMES "${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")

	ELSEIF(MINGW)

		# Works with MinGW 14.2.0.
		GET_FILENAME_COMPONENT(_MINGW_BINDIR "${CMAKE_CXX_COMPILER}" PATH)
		GET_FILENAME_COMPONENT(_MINGW_BINDIR "${_MINGW_BINDIR}/../" REALPATH)
		FIND_FILE(libstdcxx_path "libstdc++-6.dll"
			HINTS
				"${_MINGW_BINDIR}/bin"
				"${_MINGW_BINDIR}/lib"
				"${_MINGW_BINDIR}/*/bin"
				"${_MINGW_BINDIR}/*/lib"
			NO_CMAKE_FIND_ROOT_PATH
			NO_DEFAULT_PATH)
		FIND_FILE(libgcc_path
			NAMES
				"libgcc_s_seh-1.dll"
				"libgcc_s_dw2-1.dll"
				"libgcc_s_sjlj-1.dll"
			HINTS
				"${_MINGW_BINDIR}/bin"
				"${_MINGW_BINDIR}/lib"
				"${_MINGW_BINDIR}/*/bin"
				"${_MINGW_BINDIR}/*/lib"
			NO_CMAKE_FIND_ROOT_PATH
			NO_DEFAULT_PATH)
		FIND_FILE(libssp_path "libssp-0.dll"
			HINTS
				"${_MINGW_BINDIR}/bin"
				"${_MINGW_BINDIR}/lib"
				"${_MINGW_BINDIR}/*/bin"
				"${_MINGW_BINDIR}/*/lib"
			NO_CMAKE_FIND_ROOT_PATH
			NO_DEFAULT_PATH)
		FIND_FILE(libwinpthread_path "libwinpthread-1.dll"
			HINTS
				"${_MINGW_BINDIR}/bin"
				"${_MINGW_BINDIR}/lib"
				"${_MINGW_BINDIR}/*/bin"
				"${_MINGW_BINDIR}/*/lib"
			NO_CMAKE_FIND_ROOT_PATH
			NO_DEFAULT_PATH)

		IF(libstdcxx_path STREQUAL "libstdcxx_path-NOTFOUND")
			MESSAGE(FATAL_ERROR "Could not find libstdc++")
		ENDIF()

		IF(libgcc_path STREQUAL "libgcc_path-NOTFOUND")
			MESSAGE(FATAL_ERROR "Could not find libgcc")
		ENDIF()

		IF(libssp_path STREQUAL "libssp_path-NOTFOUND")
			MESSAGE(STATUS "Could not find libssp. speed-dreams-2 might be unable to run.")
		ELSE()
			SET(_COMPILER_DLL_PATHNAMES "${libssp_path}")
		ENDIF()

		IF(libwinpthread_path STREQUAL "libwinpthread_path-NOTFOUND")
			MESSAGE(STATUS "Could not find libwinpthread. win32 thread model assumed.")
		ELSE()
			SET(_COMPILER_DLL_PATHNAMES "${libwinpthread_path}")
		ENDIF()

		SET(_COMPILER_DLL_PATHNAMES
			${_COMPILER_DLL_PATHNAMES}
			"${libstdcxx_path}"
			"${libgcc_path}"
			)

	ENDIF(MSVC)

	# 5) Copy found compiler DLL files to the bin folder (for running without installing).
	FOREACH(_DLL ${_COMPILER_DLL_PATHNAMES})
		ADD_CUSTOM_COMMAND(TARGET ${TARGET_NAME} POST_BUILD
                 		   COMMAND ${CMAKE_COMMAND} -E echo Copying "${_DLL}" to "${_NOINST_DIR}"
                 		   COMMAND ${CMAKE_COMMAND} -E copy "${_DLL}" "${_NOINST_DIR}"
                 		   VERBATIM)
	ENDFOREACH()

	# 6) Install found compiler DLL files to the install folder.
	SD_INSTALL_FILES(BIN FILES ${_COMPILER_DLL_PATHNAMES})

ENDMACRO(SD_INSTALL_CUSTOM_3RDPARTY TARGET_NAME)
