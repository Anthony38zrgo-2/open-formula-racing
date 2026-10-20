#  Speed Dreams, a free and open source motorsport simulator.
#  Copyright (C) 2019 Joe Thompson, 2025 Xavier Del Campo Romero
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.

SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "speed-dreams-data(>= ${data_version})")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"Free and open source motorsport simulator
${CPACK_PACKAGE_DESCRIPTION}

This package contains the engine and tools binaries."
)
