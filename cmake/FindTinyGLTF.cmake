# Locate TinyGLTF (optionally header-only) library
# This module defines
# TINYGLTF_FOUND :
# TINYGLTF_INCLUDE_DIR : where to find the headers
# Created by Joe Thompson

find_path(TINYGLTF_INCLUDE_DIR tiny_gltf.h)

find_library(TINYGLTF_LIBRARY NAMES tinygltf)

# handle the QUIETLY and REQUIRED arguments and set TINYGLTF_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TinyGLTF DEFAULT_MSG TINYGLTF_LIBRARY TINYGLTF_INCLUDE_DIR)

#if(TINYGLTF_FOUND)
set(TINYGLTF_LIBRARIES ${TINYGLTF_LIBRARY})
#endif()
# Set the old TINYGLTF_INCLUDE_DIRS variable for backwards compatibility
set(TINYGLTF_INCLUDE_DIRS ${TINYGLTF_INCLUDE_DIR})

mark_as_advanced(TINYGLTF_LIBRARY TINYGLTF_INCLUDE_DIR)
