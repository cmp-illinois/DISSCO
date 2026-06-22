Building on Linux 
=================

Preliminary Requirements
--------------------------

The following are *necessary* to compile anything:

- git
- A C++17-supporting compiler (g++, clang),
- A C compiler (gcc ...), and
- cmake >= 3.25

To compile LASS:

- libsndfile >= 1.0,

To compile CMOD:
- muparser >= 2.X, and
- pugixml >= 1.15

> *Note*: the source code for both of these parsers are included in this repo, so you don't need to download them.
<!-- Keeping for reference, but not relevant to current development -->
<!-- To compile with LASSIE, the following couple are *necessary* inclusions:

- GTK+ 2.4 < 3.24 (developers: also should be updated!) and
- GTKmm-2.4 >= 1.5. -->

To compile LASSIE:

- Qt >= 6.8

Installing requirements and recommendations on:
-----------------------------------------------

### Debian-likes
Install the following:

    sudo apt install build-essential libsndfile1 libsndfile1-dev qt6-base-dev

`build-essential` includes `g++` and `gcc`. If you prefer another C/C++ compiler, you can install separately.

**For developers**: it's recommended you have `ccache` set up and `lld` installed:

    sudo apt install ccache lld

Since we precompile headers for LASSIE and CMOD, we suggest `export CCACHE_SLOPPINESS=pch_defines,time_macros`. Please review the `ccache` man page provided in your distribution.

<!-- TODO: RHEL, maybe -->

Installing DISSCO
-----------------
Just `git clone` this repo; explicitly:

    git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git

Building
--------
In the project's root directory (by default: `path/to/DISSCO-X.X.X`), run the following:

    mkdir build && cd build

Then, in `build`, do

    cmake ..

to generate the build files and

    cmake --build .

to build.

By running this command in `build`, one generates a so-called *out-of-source* (OOS) build. The alternative, an in-source build, is heavily discouraged (including [by the CMake maintainers](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html#directory-structure)), and the root `CMakeLists.txt` reflects this distaste. The rationale is that OOS builds minimize clutter and collect all build files in one directory, whereas in-source builds put build files virtually everywhere. (This is bad.)

From `build`, you can clean `build` using `cmake --build . --target clean`. Alternatively, you can do `rm -r build` from outside of `build`.

Building a release AppImage
---------------------------
Extra packages needed for AppImage builds:

    sudo apt install curl libfuse2 file desktop-file-utils libgl1-mesa-dev

From the project root:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    QMAKE=/usr/lib/qt6/bin/qmake6 cmake --build build --target appimage

This produces `build/DISSCO-<version>-Linux-<arch>.AppImage` — a self-contained executable with Qt, libsndfile, and CMOD bundled in. It runs on most modern Linux distros without further installation.

The first invocation downloads `linuxdeploy` and `linuxdeploy-plugin-qt` into `build/.linuxdeploy/`. The icon (`packaging/linux/LASSIE.png`) is a placeholder; replace it with real artwork before cutting a release.
