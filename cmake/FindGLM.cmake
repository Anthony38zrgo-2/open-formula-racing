# Locate GLM math (header-only) library
# This module defines
# GLM_FOUND :
# GLM_INCLUDE_DIR : where to find the headers
# Created by Joe Thompson

find_path(GLM_INCLUDE_DIR glm/glm.hpp)


# handle the QUIETLY and REQUIRED arguments and set GLM_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLM DEFAULT_MSG GLM_INCLUDE_DIR)

#if(GLM_FOUND)
set(GLM_LIBRARIES ${GLM_LIBRARY})
#endif()
# Set the old GLM_INCLUDE_DIRS variable for backwards compatibility
set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})

mark_as_advanced(GLM_LIBRARY GLM_INCLUDE_DIR)
