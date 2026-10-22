#CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12 FATAL_ERROR)
CMAKE_MINIMUM_REQUIRED(VERSION 3.12 FATAL_ERROR)

# Speed Dreams project
PROJECT(speed-dreams-2 VERSION 2.4.2
   HOMEPAGE_URL "https://www.speed-dreams.net/"
)

MESSAGE(STATUS CMAKE_VERSION = "${CMAKE_VERSION}")
SET(BUILD_SHARED_LIBS ON)

IF(POLICY CMP0003)
   CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(POLICY CMP0003)

IF(POLICY CMP0048)
   CMAKE_POLICY(SET CMP0048 NEW)
ENDIF(POLICY CMP0048)

IF(POLICY CMP0056)
   CMAKE_POLICY(SET CMP0056 NEW)
ENDIF(POLICY CMP0056)

IF(POLICY CMP0066)
  CMAKE_POLICY(SET CMP0066 NEW)
ENDIF(POLICY CMP0066)

IF(POLICY CMP0072)
  CMAKE_POLICY(SET CMP0072 NEW)
ENDIF(POLICY CMP0072)

IF(APPLE)
   SET(CMAKE_CXX_STANDARD 11)
   SET(CMAKE_CXX_STANDARD_REQUIRED True)
ENDIF(APPLE)

option(SD_ASSUME_DATADIR "Assume default directory for speed-dreams-data")
set(data_version 2.4.2)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)

INCLUDE(cmake/macros.cmake)
INCLUDE(cmake/checks.cmake)

set(CMAKE_INSTALL_RPATH
   ${CMAKE_INSTALL_RPATH}
   ${CMAKE_INSTALL_FULL_LIBDIR}
   ${CMAKE_INSTALL_PREFIX}/${SD_LIBDIR}/lib
)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

if(SD_ASSUME_DATADIR)
   set(SD_DATADIR ${CMAKE_INSTALL_DATADIR}/games/speed-dreams-2)
   set(SD_DATADIR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/games/speed-dreams-2)
   set(SD_DATADIR_ABS ${SD_DATADIR_INSTALL_PREFIX})
   message(STATUS "Data directory: ${SD_DATADIR_ABS} (assumed)")
elseif(EXISTS ${CMAKE_CURRENT_LIST_DIR}/speed-dreams-data/CMakeLists.txt)
   add_subdirectory(speed-dreams-data)
   set(SD_DATADIR ${CMAKE_INSTALL_DATADIR}/games/speed-dreams-2)
   set(SD_DATADIR_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/games/speed-dreams-2)
   set(SD_DATADIR_ABS ${CMAKE_CURRENT_SOURCE_DIR}/speed-dreams-data/data)
   message(STATUS "Data directory: ${SD_DATADIR_ABS} (in-tree)")
else()
   find_package(speed-dreams-data ${data_version} QUIET)

   if(NOT SD_DATADIR)
      message(FATAL_ERROR
         "Data directory not found or incompatible with version "
         "${data_version}. Please perform one of the following actions:\n"
         "- Update the data git submodule to do an in-tree build with the "
         " command below, and then configure the project again:\n"
         "git submodule update --init --recursive\n"
         "- If speed-dreams-data has been installed to a non-standard "
         "location, configure the project with "
         "-DCMAKE_PREFIX_PATH=<data-dir>.\n"
         "- Configure the project with -DSD_ASSUME_DATADIR=1 to assume "
         "speed-dreams-data was installed into a standard location. "
         "Speed Dreams should then still be able to build, but it might "
         "not run successfully. This option is usually only recommended "
         "for CI/CD and packagers."
      )
   endif()

   message(STATUS "Data directory: ${SD_DATADIR_ABS}")
endif()

find_package(SOLID 2.1.2)

if(NOT SOLID_FOUND)
   message(STATUS "SOLID not found. Using in-tree submodule.")
   add_subdirectory(freesolid)
endif()

if(CMAKE_CXX_COMPILER_ID)
   set(SD_BUILD_INFO_COMPILER_NAME ${CMAKE_CXX_COMPILER_ID})
else()
   set(SD_BUILD_INFO_COMPILER_NAME "Unknown")
endif()

CHECK_HEADERS_H()
CHECK_FUNCTIONS()
CHECK_LIBRARIES()

execute_process(
   COMMAND git describe --tags --dirty
   RESULT_VARIABLE result
   OUTPUT_VARIABLE VERSION
   OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(result EQUAL 0)
   set(version_regex "v([0-9]+)\.([0-9]+)\.([0-9]+).*")
   string(REGEX REPLACE
      ${version_regex}
      "\\1" CMAKE_PROJECT_VERSION_MAJOR ${VERSION})
   string(REGEX REPLACE
      ${version_regex}
      "\\2" CMAKE_PROJECT_VERSION_MINOR ${VERSION})
   string(REGEX REPLACE
      ${version_regex}
      "\\3" CMAKE_PROJECT_VERSION_PATCH ${VERSION})
else()
   set(VERSION "v${CMAKE_PROJECT_VERSION}")
endif()

set(VERSION_LONG ${VERSION} CACHE STRING "The version string displayed")

# CMAKE_PROJECT_VERSION_TWEAK is used by NSIS packagers
# its what comes after the '-' sign (if anything)
# ie: in tag '2.2.2-rc1' CMAKE_PROJECT_VERSION_TWEAK should be '-rc1'
STRING(FIND "${VERSION}" "-" _METADATA_INDEX)
IF(NOT ${_METADATA_INDEX} EQUAL -1)
   STRING(SUBSTRING ${VERSION} ${_METADATA_INDEX} -1 CMAKE_PROJECT_VERSION_TWEAK)
ELSE()
   SET(CMAKE_PROJECT_VERSION_TWEAK)
ENDIF()
#MESSAGE(STATUS "CMAKE_PROJECT_VERSION_TWEAK ${CMAKE_PROJECT_VERSION_TWEAK}")

# Generate config.h
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Generate DOxygen configuration file
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/cmake/doxygenconfig.in.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/doxygen_config @ONLY)

# Setup empty GLOBAL lists for robots and modules
SET_PROPERTY(GLOBAL PROPERTY SD_MODULE_LIST "")
SET_PROPERTY(GLOBAL PROPERTY SD_CAR_LIST "")
SET_PROPERTY(GLOBAL PROPERTY SD_TRACK_LIST "")
SET_PROPERTY(GLOBAL PROPERTY SD_ROBOT_LIST "")
SET_PROPERTY(GLOBAL PROPERTY SD_OSG_PLUGIN_LIST "")
SET_PROPERTY(GLOBAL PROPERTY SD_EXE_LIST "")

# Sub-dirs to build ...
ADD_SUBDIRECTORY(src)

add_executable(inspect_native_track tools/inspect_native_track.cpp)
target_include_directories(inspect_native_track PRIVATE src/interfaces src/libs/tgf src/libs/portability)
target_link_libraries(inspect_native_track PRIVATE tgf)

add_executable(validate_formula_pedal_controls tools/validate_formula_pedal_controls.cpp src/libs/robottools/formula_gear_controls.cpp)
target_compile_definitions(validate_formula_pedal_controls PRIVATE FORMULA_GEAR_CONTROLS_STATIC)
ADD_PLIB_INCLUDEDIR()
target_include_directories(validate_formula_pedal_controls PRIVATE src/interfaces src/libs/tgf src/libs/portability src/libs/robottools)
target_link_libraries(validate_formula_pedal_controls tgf)
ADD_SDL2_INCLUDEDIR()
ADD_SDL2_LIBRARY(validate_formula_pedal_controls)

add_executable(validate_formula_aerodynamics tools/validate_formula_aerodynamics.cpp
    src/modules/simu/simuv5/aero.cpp src/modules/simu/simuv5/simulationOptions.cpp)
target_include_directories(validate_formula_aerodynamics PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools freesolid/include)
target_link_libraries(validate_formula_aerodynamics tgf)

add_executable(validate_formula_differential tools/validate_formula_differential.cpp src/modules/simu/simuv5/differential.cpp)
target_include_directories(validate_formula_differential PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools freesolid/include)
target_link_libraries(validate_formula_differential tgf)

add_executable(validate_williams_chassis tools/validate_williams_chassis.cpp
    src/modules/simu/simuv5/axle.cpp src/modules/simu/simuv5/susp.cpp)
target_include_directories(validate_williams_chassis PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools freesolid/include)
target_link_libraries(validate_williams_chassis tgf)

add_executable(validate_formula_tire_forces tools/validate_formula_tire_forces.cpp
    src/modules/simu/simuv5/tire_force_model.cpp
    src/modules/simu/simuv5/tire_model_configuration.cpp)
target_include_directories(validate_formula_tire_forces PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools)
target_link_libraries(validate_formula_tire_forces tgf)

add_executable(validate_formula_tire_contact tools/validate_formula_tire_contact.cpp
    src/modules/simu/simuv5/tire_contact_model.cpp
    src/modules/simu/simuv5/tire_thermal_model.cpp
    src/modules/simu/simuv5/tire_model_configuration.cpp)
target_include_directories(validate_formula_tire_contact PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools)
target_link_libraries(validate_formula_tire_contact tgf)

add_executable(validate_formula_tire_thermal tools/validate_formula_tire_thermal.cpp
    src/modules/simu/simuv5/tire_thermal_model.cpp
    src/modules/simu/simuv5/tire_wear_model.cpp
    src/modules/simu/simuv5/tire_model_configuration.cpp)
target_include_directories(validate_formula_tire_thermal PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools)
target_link_libraries(validate_formula_tire_thermal tgf)

add_executable(validate_formula_tire_selector tools/validate_formula_tire_selector.cpp
    src/modules/simu/simuv5/tire_model_configuration.cpp)
target_include_directories(validate_formula_tire_selector PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools)
target_link_libraries(validate_formula_tire_selector tgf)

add_executable(validate_formula_tire_geometry tools/validate_formula_tire_geometry.cpp
    src/modules/simu/simuv5/tire_vertical_geometry.cpp)
target_include_directories(validate_formula_tire_geometry PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools)
target_link_libraries(validate_formula_tire_geometry tgf)

add_executable(validate_formula_tire_vertical_system tools/validate_formula_tire_vertical_system.cpp
    src/modules/simu/simuv5/tire_vertical_solver.cpp
    src/modules/simu/simuv5/tire_vertical_geometry.cpp
    src/modules/simu/simuv5/tire_model_configuration.cpp
    src/modules/simu/simuv5/susp.cpp
    src/modules/simu/simuv5/axle.cpp)
target_include_directories(validate_formula_tire_vertical_system PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools freesolid/include)
target_link_libraries(validate_formula_tire_vertical_system tgf)

add_executable(validate_rigid_contact_safety tools/validate_rigid_contact_safety.cpp
    src/modules/simu/simuv5/tire_vertical_solver.cpp)
target_include_directories(validate_rigid_contact_safety PRIVATE src/modules/simu/simuv5)

add_executable(validate_formula_vertical_authority tools/validate_formula_vertical_authority.cpp
    src/modules/simu/simuv5/car.cpp
    src/modules/simu/simuv5/collide.cpp
    src/modules/simu/simuv5/susp.cpp
    src/modules/simu/simuv5/axle.cpp
    src/modules/simu/simuv5/wheel.cpp
    src/modules/simu/simuv5/simulationOptions.cpp
    src/modules/simu/simuv5/tire_model.cpp
    src/modules/simu/simuv5/tire_model_configuration.cpp
    src/modules/simu/simuv5/tire_contact_model.cpp
    src/modules/simu/simuv5/tire_force_model.cpp
    src/modules/simu/simuv5/tire_thermal_model.cpp
    src/modules/simu/simuv5/tire_wear_model.cpp
    src/modules/simu/simuv5/tire_vertical_geometry.cpp
    src/modules/simu/simuv5/tire_vertical_solver.cpp
    src/modules/simu/simuv5/engine.cpp
    src/modules/simu/simuv5/transmission.cpp
    src/modules/simu/simuv5/differential.cpp
    src/modules/simu/simuv5/steer.cpp
    src/modules/simu/simuv5/brake.cpp
    src/modules/simu/simuv5/aero.cpp
    src/modules/simu/simuv5/track_ground_mesh.cpp
    src/modules/simu/simuv5/categories.cpp)
target_include_directories(validate_formula_vertical_authority PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools freesolid/include)
target_link_libraries(validate_formula_vertical_authority tgf robottools solid)
set_target_properties(validate_formula_vertical_authority PROPERTIES ENABLE_EXPORTS FALSE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(validate_formula_vertical_authority PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(validate_formula_vertical_authority PRIVATE -Wl,--gc-sections -Wl,--exclude-all-symbols)
endif()

add_executable(validate_formula_tire_energy tools/validate_formula_tire_energy.cpp
    src/modules/simu/simuv5/tire_thermal_model.cpp
    src/modules/simu/simuv5/tire_model_configuration.cpp)
target_include_directories(validate_formula_tire_energy PRIVATE src/modules/simu/simuv5
    src/interfaces src/libs/tgf src/libs/portability src/libs/math src/libs/robottools)
target_link_libraries(validate_formula_tire_energy tgf)

if(WIN32)
    add_executable(OpenFormula WIN32 tools/open_formula_launcher.cpp)
    set_target_properties(OpenFormula PROPERTIES CXX_STANDARD 17)
    if(MINGW)
        target_link_options(OpenFormula PRIVATE -municode -static -static-libgcc -static-libstdc++)
    endif()
    install(TARGETS OpenFormula RUNTIME DESTINATION .)
endif()

IF(MSVC)
   INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/doc/faq/faq.html DESTINATION ./doc)
   INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/doc/userman/how_to_drive.html DESTINATION ./doc)
   INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/userman/images/ DESTINATION ./doc/images/)
ENDIF(MSVC)

if(WIN32)
   function(copy_dll target)
      add_custom_target(${target}_copydll)
      add_dependencies(${target}_copydll ${target})
      add_dependencies(${PROJECT_NAME} ${target}_copydll)
      add_custom_command(TARGET ${target}_copydll
         COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${target}> ${CMAKE_BINARY_DIR}/bin/
         VERBATIM
      )
   endfunction()

   copy_dll(solid)
   copy_dll(broad)
   copy_dll(moto)
endif()

# Must be the last ADD_SUBDIRECTORY for FIXUP_BUNDLE to work
ADD_SUBDIRECTORY(packaging)

IF(OPTION_CHECK_CONTENTS)
   INCLUDE(cmake/prerelease.cmake)
ENDIF(OPTION_CHECK_CONTENTS)

INCLUDE(cpack/common.cmake)
INCLUDE(CPack)
