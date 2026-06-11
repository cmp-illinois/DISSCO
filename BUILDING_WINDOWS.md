Building on Windows 
=================
(WIP, not yet building without some hacking)

Preliminary Requirements
--------------------------

The following are *necessary* to anything:

- git,
- A C/C++ compiler (like MSVC `cl` or `clang-cl`), and
- cmake >= 3.25
<!-- - muparser >= 2.X -->

To compile LASS:

- [libsndfile](https://libsndfile.github.io/libsndfile/#download) >= 1.0,

To compile CMOD:
- muparser >= 2.X, and
- pugixml >= 1.15

> *Note*: the source code for both of these parsers are included in this repo, so you don't need to download them.

To compile LASSIE:

- [Qt](https://www.qt.io/development/download-qt-installer-oss) >= 6.8

> *Note*: there are several ways to get these packages onto your system. This doc will go through a fairly simple one.

Installing requirements and recommendations:
--------------------------------------------

When installing Qt via the graphical installer, you'll have the option to install additional tools. These include **CMake** and a **compiler**. Please choose a compiler for your computer's architecture (x86-64 for Intel and ARM for Arm).

Please be aware that the compiler architecture must match the architecture of the Qt version you install. You will get an error at link-time if they differ.

Installing DISSCO
-----------------
Just `git clone` this repo; explicitly:

    git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git

Building
--------

From the project's root directory (by default: `C:\path\to\DISSCO-X.X.X`), configure the build:

    cmake -S . -B build -G Ninja

If you installed `libsndfile` somewhere outside the standard locations, point CMake at it:

    cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="C:/path/to/libsndfile"

Then, from `build/`, do

    cmake --build .

to build.

By running this command in `build`, one generates a so-called *out-of-source* (OOS) build. The alternative, an in-source build, is heavily discouraged (including [by the CMake maintainers](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html#directory-structure)), and the root `CMakeLists.txt` reflects this distaste. The rationale is that OOS builds minimize clutter and collect all build files in one directory, whereas in-source builds put build files virtually everywhere. (This is bad.)

From `build`, you can clean `build` using `cmake --build . --target clean`. Alternatively, you can do `rmdir /s build` from outside of `build`.

Building a release installer
----------------------------
The release pipeline produces an NSIS installer `.exe` that bundles `LASSIE.exe`, `CMOD.exe`, the Qt runtime, and writes registry entries for the `.dissco` file association.

Extra requirement: **NSIS** (Nullsoft Scriptable Install System).

    choco install nsis

Then from Developer PowerShell at the project root:

    cmake -S . -B build -G Ninja `
        -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    cmake --build build --target package

This produces `build/DISSCO-<version>-Windows.exe`. Under the hood:

- CPack's NSIS generator wraps the install tree into a single-file installer.
- `windeployqt` runs at install time against the installed `LASSIE.exe` and copies the Qt DLLs + platform plugins next to it.
- The installer writes `HKLM\Software\Classes\.dissco` registry entries so `.dissco` files open LASSIE on double-click immediately after install (system-wide). If the file-association branch (`claude/sad-hermann-4b5591`) is merged, LASSIE.exe will additionally self-register per-user (HKCU) on first launch — both layers describe the same ProgID.
- The installer also installs a Start Menu shortcut and an optional desktop shortcut.

The icon (`packaging/windows/LASSIE.ico`) is a placeholder; regenerate it from updated artwork via `packaging/windows/make-ico.sh`.

**Code signing:** unsigned `.exe` installers trigger Windows SmartScreen warnings on download. Production releases should be signed with an EV or OV code-signing certificate. The GitHub Actions release workflow has a clean place to wire signtool in once the certificate is provisioned.