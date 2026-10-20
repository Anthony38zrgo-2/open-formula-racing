# Third party dependency builds for Speed Dreams

On platforms where a package manager is not available, such as Microsoft
Windows, third party dependencies must be built from source so CMake can find
them when building Speed Dreams. Builds are based on the
[`ExternalProject`](https://cmake.org/cmake/help/latest/module/ExternalProject.html)
CMake module.

Therefore, Speed Dreams holds [a list](./thirdpartydefinitions.cmake) of
pinnings against third-party dependencies. A build consists of:

1. Fetching the source code from different repositories.
2. Building the dependencies.
3. Installing them to the `3rdParty/` directory, inside the build directory.
**This is the directory referred to by the top-level**
**[`CMakeLists.txt`](../../CMakeLists.txt)**
**when performing Windows builds, via either `CMAKE_PREFIX_PATH` or**
**`CMAKE_FIND_ROOT_PATH`**. Read
[the relevant section from the top-level `README.md`](../../README.md#windows)
for further reference.

## Requirements

### Debian/Ubuntu

```
sudo apt install cmake subversion git
```

A modern version of the C++ compiler is required because
[OpenAL](https://github.com/kcat/openal-soft) makes use of C++11's
`std::mutex`, which was added to `win32` only recently.
So far, only `14.2.0` has been successfully tested.
Therefore, **this might require to build GCC from source,**
**until distributions ship more modern versions of the C++ compiler**.

#### `x86_64`

```
sudo apt install g++-mingw-w64-x86-64-win32
```

#### `i686`

```
sudo apt install g++-mingw-w64-i686-win32
```

### Windows

- [CMake](https://cmake.org/download/#latest) `3.5.0` or newer
- [Git](https://git-scm.com/download/win)
- [TortoiseSVN](https://tortoisesvn.net/)
- [MinGW](https://winlibs.com/), with support for C++11 and `std::mutex`.
So far, `14.2.0` has been tested successfully.

> Please ensure these tools are added to the `%PATH%` environment variable.

## Configuring

### Natively from Windows

```
cmake -B build
```

### Cross-building from Linux

CMake defines the
[`CMAKE_TOOLCHAIN_FILE`](https://cmake.org/cmake/help/latest/variable/CMAKE_TOOLCHAIN_FILE.html)
variable to set up the cross-toolchain. This directory defines several files,
according to the target platform:

#### `x86_64`

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=x86_64-w64-mingw32.cmake
```

#### `i686`

```
cmake -B build -DCMAKE_TOOLCHAIN_FILE=i686-w64-mingw32.cmake
```

## Building

Once the project is configured according to the steps above, build with:

```
cmake --build build/ # Optionally, add -j8 or any other number for faster builds
```

The build will probably take a long time, as it builds tens of third-party
libraries fetched from multiple sources. Once finished, a directory called
`3rdParty/` should be available inside the build directory, ready to be
consumed by [the top-level `CMakeLists.txt`](../../CMakeLists.txt).
