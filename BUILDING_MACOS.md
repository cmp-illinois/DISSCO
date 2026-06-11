Building on macOS 
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
 
Installing requirements and recommendations:
--------------------------------------------

<!-- *For all methods*, it's worth keeping in mind that we statically link `muparser`, meaning you don't need to worry about installing it. Please report any issues related to muparser during compilation. -->

### brew
`brew`, or homebrew, is something like a package manager for macOS. It's widely used and tends to have better maintained packages than the equivalent ports on MacPorts. Read more and acquire it at [https://brew.sh](https://brew.sh). Simply:

    $ brew install libsndfile qt@6

### port
`port`, or MacPorts, is something like BSD ports for macOS. It's widely used, but less so than Homebrew. Read more and acquire it at [https://macports.org](https://macports.org). Simply, with `sudo`/root:

    # port install libsndfile qt6

For ARM Macs, `brew` will install in `/opt/homebrew/Cellar/[your stuff here]` and symlink to `/opt/homebrew/opt/[your stuff here]`, and for Intel/x86 Macs, `brew` will install in `/usr/local/bin/[your stuff here]`.

MacPorts will install it in `/opt/local/include/[your includes here]` and `/opt/local/lib/[your libs here]`.

**For developers**: it's recommended you have `ccache` set up and `lld` installed:

    $ brew install ccache lld

(MacPorts only packages `ccache` individually--`lld` is included in the (large) llvm package.)

Since we precompile headers for LASSIE and CMOD, we suggest `export CCACHE_SLOPPINESS=pch_defines,time_macros`. Please review the `ccache` man page for more.


Installing DISSCO
-----------------
Just `git clone` this repo; explicitly:

    git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git

Building
--------
From the project's root directory (by default: `path/to/DISSCO-X.X.X`), run the following:

    mkdir build && cd build

Then, in `build`, do

    cmake ..

to generate the build files and

    cmake --build .

to build.

By running this command in `build`, one generates a so-called *out-of-source* (OOS) build. The alternative, an in-source build, is heavily discouraged (including [by the CMake maintainers](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html#directory-structure)), and the root `CMakeLists.txt` reflects this distaste. The rationale is that OOS builds minimize clutter and collect all build files in one directory, whereas in-source builds put build files virtually everywhere. (This is bad.)

From `build`, you can clean `build` using `cmake --build . --target clean`. Alternatively, you can do `rm -r build` from outside of `build`. Yet another option, from within build: `rm CMakeCache.txt`.

Building a release DMG
----------------------
From the project root:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"
    cmake --build build --parallel
    cmake --build build --target package

This produces `build/DISSCO-<version>-Darwin.dmg`. The DMG contains `LASSIE.app` with the CMOD binary embedded at `Contents/MacOS/CMOD` and Qt frameworks bundled in via `macdeployqt`.

The build's icon (`packaging/macos/LASSIE.icns`) is a placeholder; regenerate it from updated artwork via `packaging/macos/make-icns.sh`.

**Note on Gatekeeper**: without an Apple Developer ID signature + notarization, users opening the DMG will see a "cannot be verified" dialog and must right-click → Open. Code-signing and notarization can be added to the GitHub Actions release workflow once Apple Developer credentials are available.
