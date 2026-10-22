INCLUDE(../../../../cmake/macros.cmake)

ADD_SDLIB_INCLUDEDIR(portability)

IF(WIN32)
	# DLL export stuff under Windows (to avoid .def file)
	ADD_DEFINITIONS(-DRUSTPHYSICS_DLL)
ENDIF(WIN32)

ADD_PLIB_INCLUDEDIR()

ADD_INTERFACE_INCLUDEDIR()

ADD_SDLIB_INCLUDEDIR(math tgf robottools)

SET(RUSTPHYSICS_SOURCES rustphysics.cpp rust_library.cpp)
SET(RUSTPHYSICS_HEADERS rustphysics.h rust_library.h rust_abi.h)

SD_ADD_LIBRARY(rustphysics MODULE ${RUSTPHYSICS_SOURCES} ${RUSTPHYSICS_HEADERS})

ADD_SDLIB_LIBRARY(rustphysics portability tgf robottools)

SD_INSTALL_FILES(LIB modules/simu TARGETS rustphysics)

# The physics implementation lives in a self-contained Rust cdylib loaded through its
# versioned C ABI. It is rebuilt from <source>/rust as part of this module so the host
# and the library can never be installed with mismatched structure declarations; the
# loader additionally verifies the ABI version and layout signature at start-up.
SET(RUST_PHYSICS_DIR "${CMAKE_SOURCE_DIR}/rust")
SET(RUST_PHYSICS_DLL "${RUST_PHYSICS_DIR}/target/release/vehicle_physics_engine.dll")

FIND_PROGRAM(CARGO_EXECUTABLE cargo REQUIRED)
ADD_CUSTOM_TARGET(rust_physics_library ALL
    COMMAND "${CARGO_EXECUTABLE}" build --offline --release
    WORKING_DIRECTORY "${RUST_PHYSICS_DIR}"
    BYPRODUCTS "${RUST_PHYSICS_DLL}"
    COMMENT "Checking and building Rust physics dependencies with Cargo"
    VERBATIM)
ADD_DEPENDENCIES(rustphysics rust_physics_library)
SD_INSTALL_FILES(LIB modules/simu FILES "${RUST_PHYSICS_DLL}")
