Building on macOS 
=================

Preliminary Requirements
--------------------------

The following are *necessary* to compile CMOD and LASS:

- git
- A C++17-supporting compiler (g++, clang++),
- A C compiler (gcc, clang),
- cmake >= 3.25,
- libsndfile >= 1.0,
- libxerces-c >= 3.2, and

To compile with LASSIE, the following is *necessary*:

- Qt >= 6.8
 
Installing requirements and recommendations:
--------------------------------------------

<!-- *For all methods*, it's worth keeping in mind that we statically link `muparser`, meaning you don't need to worry about installing it. Please report any issues related to muparser during compilation. -->

### brew
`brew`, or homebrew, is something like a package manager for macOS. It's widely used and tends to have better maintained packages than the equivalent ports on MacPorts. Read more and acquire it at [https://brew.sh](https://brew.sh). Simply:

    $ brew install libsndfile xerces-c qt@6

### port
`port`, or MacPorts, is something like BSD ports for macOS. It's widely used, but less so than Homebrew. Read more and acquire it at [https://macports.org](https://macports.org). Simply, with `sudo`/root:

    # port install libsndfile xercesc3 qt6

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

`.dissco` File Association
--------------------------

LASSIE's macOS bundle can claim `.dissco` files via Launch Services so that double-clicking a project in Finder opens it in LASSIE. This is **only enabled in release builds** (or when you explicitly pass `-DLASSIE_CLAIM_DISSCO=ON`) so that multiple debug/worktree bundles on the same machine don't fight over the association.

    cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release
    cmake --build . --target LASSIE
    cp -R LASSIE/LASSIE.app /Applications/        # or: lsregister -f LASSIE/LASSIE.app

After this, right-clicking a `.dissco` in Finder should offer LASSIE under "Open With", and `mdls -name kMDItemContentTypeTree some.dissco` should list `org.dissco.project`.
