Building on Linux 
=================

Preliminary Requirements
--------------------------

The following are *necessary* to compile CMOD and LASS:

- git
- A C++17-supporting compiler (g++, clang),
- A C compiler (gcc ...),
- cmake >= 3.5,
- libsndfile >= 1.0,
- libxerces-c >= 3.2, and
<!-- - muparser >= 2.X -->

<!-- Keeping for reference, but not relevant to current development -->
<!-- To compile with LASSIE, the following couple are *necessary* inclusions:

- GTK+ 2.4 < 3.24 (developers: also should be updated!) and
- GTKmm-2.4 >= 1.5. -->

To compile with LASSIE, the following is *necessary*:

- Qt >= 6.8

Recommended
-----------

LASSIE will want to open up a terminal, either gnome-terminal or xterm, to provide an interactive view of CMOD running your program. One of these is likely installed already if you're using a desktop environment. Otherwise, install one or the other, or both. *Note that LASSIE is an X application.*

Installing requirements and recommendations on:
-----------------------------------------------

### Debian-likes
Install the following:

    sudo apt install build-essential libsndfile1 libsndfile1-dev libxerces-c3.2 libxerces-c-dev qt6-base-dev

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

`.dissco` File Association
--------------------------

LASSIE can register itself as the system handler for `.dissco` files so that double-clicking a project in your file manager opens it in LASSIE. This is **only enabled in release builds** (or when you explicitly pass `-DLASSIE_CLAIM_DISSCO=ON`) so that debug/worktree builds don't fight over the association.

To install the association system-wide (or to a prefix):

    cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
    cmake --build .
    sudo cmake --install .                     # or: cmake --install . --prefix /your/prefix
    sudo update-desktop-database
    sudo update-mime-database /usr/share/mime  # adjust path if you used a custom prefix

After this, `xdg-mime query default application/x-dissco` should return `LASSIE.desktop`.
