Building on Windows
=================

Preliminary Requirements
--------------------------

The following are *necessary* to compile CMOD and LASS:

- git
- cmake >= 3.25,
- vcpkg,
- LLVM (`lld`, `clang`, `llvm-objdump`, ...),
- libsndfile >= 1.0,
- libxerces-c >= 3.2, and
<!-- - muparser >= 2.X -->

To compile with LASSIE, the following is *necessary*:

- Qt >= 6.8 (the **MSVC 2022** variant matching your CPU architecture)

Note: There are definitely other ways to compile DISSCO on Windows, but this is the way the author decided to go. In particular, the MinGW variant of Qt will *not* link against an MSVC/clang-cl toolchain, so make sure your installs match your compiler and architecture.

Installing requirements and recommendations:
--------------------------------------------

<!-- *For all methods*, it's worth keeping in mind that we statically link `muparser`, meaning you don't need to worry about installing it. Please report any issues related to muparser during compilation. -->


### LLVM (via winget)

Install the LLVM compiler suite:

    winget install LLVM.LLVM

After install, ensure `LLVM\bin` is in your `$env:Path`. You may also want to set your `CC`, `CXX`, and `RC` environment variables to `clang`, `clang++`, and `llvm-rc`, respectively.

### vcpkg

Dependency management is handled via `vcpkg`. Clone and bootstrap it somewhere persistent (e.g. `C:\vcpkg`):

    git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
    cd C:\vcpkg
    .\bootstrap-vcpkg.bat

Then set the environment variable `VCPKG_ROOT` to `C:\vcpkg` (or wherever), and add `vcpkg` to your `$env:Path`.

### Qt 6

Install Qt with the [Qt online installer](https://www.qt.io/download-qt-installer). When the component selector appears, expand the latest Qt >6.10 release and pick **one** variant that matches your CPU architecture:

- **MSVC 2022 64-bit** — if you're on Intel/AMD x64 (most desktops/laptops).
- **MSVC 2022 ARM64** — if you're on Windows-on-ARM (Surface Pro X / 11, Snapdragon X laptops, etc.).

To figure out which one you are: `echo $env:PROCESSOR_ARCHITECTURE` in PowerShell. `AMD64` → x64; `ARM64` → ARM64.

After install, note the Qt prefix path. The default is something like `C:\Qt\6.8.1\msvc2022_64` (or `…\msvc2022_arm64`). We'll point CMake at this directory in the Building section.

Installing DISSCO
-----------------
Just `git clone` this repo; explicitly:

    git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git

Building
--------

### Declare dependencies for vcpkg

From the project's root directory (by default: `C:\path\to\DISSCO-X.X.X`), tell vcpkg what we need:

    vcpkg new --application

which creates a `vcpkg-configuration.json` file, and

    vcpkg add port libsndfile
    vcpkg add port xerces-c

which creates a `vcpkg.json` and adds the two ports to it. These files are local [manifests](https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode), and vcpkg will auto-install the listed deps the first time CMake configures.

### Wire up a CMakeUserPresets.json

A `vcpkg` configure preset already lives in `CMakePresets.json` at the repo root — it points CMake at the vcpkg toolchain file so that `find_package(SndFile)` and `find_package(XercesC)` resolve to vcpkg's installs. You'll add a *user* preset on top of it that supplies the two paths that are specific to your machine: `VCPKG_ROOT` and `CMAKE_PREFIX_PATH` (the Qt install dir).

Create a `CMakeUserPresets.json` in the repo root (this filename is gitignored — it's per-developer):

```json
{
    "version": 2,
    "configurePresets": [
        {
            "name": "windows",
            "inherits": "vcpkg",
            "environment": {
                "VCPKG_ROOT": "C:/vcpkg"
            },
            "cacheVariables": {
                "CMAKE_PREFIX_PATH": "C:/Qt/6.8.1/msvc2022_64"
            }
        }
    ]
}
```

Substitute your actual `vcpkg` and Qt paths. The Qt path should be the directory containing `bin\`, `lib\cmake\Qt6\` etc. — `msvc2022_arm64` instead of `msvc2022_64` if that's what you installed.

### Configure and build

In the project root:

    cmake --preset windows
    cmake --build build

`cmake --preset windows` will, on first run, trigger vcpkg to download and build `libsndfile` and `xerces-c` (plus their transitive deps — FLAC, ogg, opus, etc.). This takes a while the first time and is cached afterward (so it'll be faster, hopefully :).

Then `cmake --build build` produces:

- `build\LASS\LASS.lib` — additive synthesis library (static),
- `build\external-libs\muParser\MUPARSER.lib` — expression parser (static),
- `build\CMOD\CMOD.exe` — composition module (CLI binary),
- `build\LASSIE\LASSIE.exe` — Qt GUI editor.

The LASSIE link step automatically runs `windeployqt` to copy `Qt6Widgets.dll`, `Qt6Core.dll`, `Qt6Gui.dll`, the platform plugin, and friends next to `LASSIE.exe`. vcpkg's applocal step does the same for `sndfile.dll`, `xerces-c*.dll`, and their deps. After the build finishes, `build\LASSIE\LASSIE.exe` should launch by double-click.

### Cleaning

From the project root: `Remove-Item -Recurse build` (PowerShell) or `rmdir /s build` (cmd). Or from inside `build`: `cmake --build . --target clean`.

By running `cmake --preset windows` (which sets `binaryDir` to `${sourceDir}/build`), one generates a so-called *out-of-source* (OOS) build. The alternative, an in-source build, is heavily discouraged (including [by the CMake maintainers](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html#directory-structure)), and the root `CMakeLists.txt` reflects this distaste. The rationale is that OOS builds minimize clutter and collect all build files in one directory, whereas in-source builds put build files virtually everywhere. (This is bad.)

`.dissco` File Association
--------------------------

LASSIE registers itself as the handler for `.dissco` files automatically on first launch. The first time you run a release build of `LASSIE.exe`, it writes (per-user, no admin required):

    HKCU\Software\Classes\.dissco                      (Default) = "DISSCO.Project"
    HKCU\Software\Classes\DISSCO.Project               (Default) = "DISSCO Project"
    HKCU\Software\Classes\DISSCO.Project\shell\open\command
                                                       (Default) = "<path-to-LASSIE.exe>" "%1"

After that, double-clicking a `.dissco` in Explorer opens LASSIE.

The self-registration is gated by `LASSIE_CLAIM_DISSCO`, which **defaults to ON only for release-type builds** (`Release`, `RelWithDebInfo`, `MinSizeRel`). Debug or worktree builds will not touch the registry, so multiple in-flight LASSIE checkouts on the same machine won't fight over the extension. To force the behavior on or off, configure with `-DLASSIE_CLAIM_DISSCO=ON|OFF`.

If you move `LASSIE.exe` to a new path, just launch it once from the new path — the registration is idempotent and updates the command line to point at the current exe. To remove it manually, delete `HKCU\Software\Classes\.dissco` and `HKCU\Software\Classes\DISSCO.Project` with `regedit`.

A manual-install fallback `.reg` lives at [packaging/windows/dissco-association.reg](packaging/windows/dissco-association.reg) for offline/scripted setups where you'd rather not launch LASSIE first. Edit the placeholder exe path inside and double-click to import.
