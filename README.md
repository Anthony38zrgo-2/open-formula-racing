# Speed Dreams

![Speed Dreams logo](./doc/readme/sd-logo.png)

Speed Dreams is a free and open source motorsport simulator. Originally
a fork of the [TORCS](https://torcs.sourceforge.net/) project,
it has evolved into a higher level of maturity, featuring realistic physics
with tens of high-quality cars and tracks to choose from.

> **This repository contains the engine source code,**
> **and the base assets [as a submodule](./speed-dreams-data/).**
> **End users are expected to download the pre-built packages listed below.**
>
> The assets repository is located on
> https://forge.a-lec.org/speed-dreams/speed-dreams-data/

![A collage of in-game screenshots](./doc/readme/collage.jpg)

## Pre-built packages

Speed Dreams binaries are available from the
[official website](https://www.speed-dreams.net/en/downloads).

## Building from source

In-tree builds (i.e., including both code and data on the same build)
are recommended for simplicity. To achieve this, update the
[`speed-dreams-data/`](./speed-dreams-data/) submodule with:

```
git submodule update --init --recursive
```

Otherwise, CMake will search the `speed-dreams-data` package on a
well-known location, or rely on the
[`CMAKE_PREFIX_PATH`](https://cmake.org/cmake/help/latest/variable/CMAKE_PREFIX_PATH.html)
variable to search for an **installed** version of the `speed-dreams-data`
package, which must then be indicated when configuring the project:

```
cmake -DCMAKE_PREFIX_PATH=<path-to-speed-dreams-data> # other args...
```

Nonetheless, there is also the [`freesolid`](./freesolid/) submodule that might
be required since that project has not been packaged by most distributions yet.
In such case, update the submodule with:

```
git submodule update --init freesolid
```

> **The steps below will assume in-tree builds unless noted otherwise.**

Once configured as described below, the project can be built with:

```
cmake --build build/ # Optionally, add -j8 or any other number for faster builds
```

Optionally, the project can be installed via:

```
cmake --install build/
```

### BSDs, Linux

Speed Dreams can be built from source using the conventional build process
in CMake projects:

```
cmake -B build
```

### Windows

- [CMake](https://cmake.org/download/#latest) `3.12.0` or newer
- [Git](https://git-scm.com/download/win)
- [OpenJDK](https://jdk.java.net/) `8` or newer (for the track editor)
- [MinGW](https://winlibs.com/), with support for C++11.

> Please ensure these tools are added to the `%PATH%` environment variable.

On Windows, dependencies cannot be fetched from a package manager as in
conventional Linux distributions. To solve this, Speed Dreams defines
[a separate build system for third-party dependencies](./packaging/3rdParty-devel/).
This would build all dependencies from source, so they can be later found
by CMake.

> **Both methods described below will require the path to the pre-built**
> **dependencies**.
> Please read [the instructions](./packaging/3rdParty-devel/README.md)
> for further reference.

#### Native builds

If building from Windows, append the path to the pre-built dependencies to
the
[`CMAKE_PREFIX_PATH`](https://cmake.org/cmake/help/latest/variable/CMAKE_PREFIX_PATH.html)
variable:

```
cmake -B build -DCMAKE_PREFIX_PATH=<path-to-dependencies>
```

If using MinGW, then the `MinGW Makefiles` CMake generator might have to be
manually selected:

```
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=<path-to-dependencies>
```

#### Cross-compiling from Linux

Speed Dreams can not only be compiled natively from Windows, but can be also
cross-compiled for Windows from a fully free (as in freedom) operating
system like GNU/Linux. The
[`w64-mingw32`](https://git.code.sf.net/p/mingw-w64/mingw-w64)
toolchain can be used for the cross-compilation, with dedicated
toolchain files for
[`i686-w64-mingw32`](./packaging/3rdParty-devel/i686-w64-mingw32.cmake)
and
[`x86_64-w64-mingw32`](./packaging/3rdParty-devel/x86_64-w64-mingw32.cmake),
referred to by the
[`CMAKE_TOOLCHAIN_FILE`](https://cmake.org/cmake/help/latest/variable/CMAKE_TOOLCHAIN_FILE.html)
variable. Third-party dependencies are then referred to by the
[`CMAKE_FIND_ROOT_PATH`](https://cmake.org/cmake/help/latest/variable/CMAKE_FIND_ROOT_PATH.html)
variable, as intended for cross-builds.

> **`CMAKE_FIND_ROOT_PATH` must be an absolute path.**

##### `x86_64`

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=packaging/3rdParty-devel/x86_64-w64-mingw32.cmake -DCMAKE_FIND_ROOT_PATH=<path-to-dependencies>
```

#### `i686`

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=packaging/3rdParty-devel/i686-w64-mingw32.cmake -DCMAKE_FIND_ROOT_PATH=<path-to-dependencies>
```

### Dependencies

#### Debian/Ubuntu

```sh
sudo apt install git cmake build-essential libopenscenegraph-dev libcurl4-gnutls-dev libsdl2-dev libsdl2-mixer-dev librhash-dev libenet-dev libpng-dev libjpeg-dev zlib1g-dev libminizip-dev libopenal-dev libplib-dev libexpat1-dev libcjson-dev openjdk-17-jdk openjdk-17-jre libsdl2-ttf-dev
```

> The version for the `openjdk-*` packages might change among distributions.

#### Fedora

```sh
sudo dnf install SDL2_mixer SDL2_mixer-devel expat-devel OpenSceneGraph-devel plib-devel libcurl-devel openal-soft-devel enet-devel minizip-devel rhash-devel mesa-libGL-devel mesa-libGLU-devel
```

## License

By default, Speed Dreams code is licensed under the GPLv2-or-later license,
as specified by the [`LICENSE`](./LICENSE) file, whereas non-functional data
is licensed under the [Free Art License](http://artlibre.org/) by default.

However, some sections of the code and some other assets are distributed under
various free (as in freedom) licenses. Please read their license files
located in their respective directories for further reference.

## Trademark disclaimer

Windows is a registered trademark of Microsoft Corporation.

Ubuntu is a registered trademark of Canonical Ltd.

Linux is a registered trademark of Linus Torvalds.
