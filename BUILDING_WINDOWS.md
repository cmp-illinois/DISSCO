Building on Windows
===================

This guide explains how to build and run DISSCO on Windows using MSVC, CMake, Ninja, Qt, vcpkg, Git for Windows, and LilyPond.

DISSCO contains three main parts:

- **LASS**: sound synthesis library
- **CMOD**: composition module
- **LASSIE**: Qt GUI for editing `.dissco` files

LASSIE can run CMOD, and CMOD can generate LilyPond score output.

Before You Start
----------------

Use **x64 Native Tools Command Prompt for VS** for all build commands.

This guide uses **CMD syntax**. PowerShell uses different syntax for line continuation and environment variables.

The commands below assume these default paths:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
set "VCPKG_ROOT=C:\dev\vcpkg"
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
set "LILYPOND_BIN=C:\Program Files\LilyPond\usr\bin"
```

You may install these tools anywhere. If your paths are different, update the `set` commands and use your paths consistently.

Required Software
-----------------

Install:

- Git for Windows
- Visual Studio or Visual Studio Build Tools with **Desktop development with C++**
- CMake 3.25 or newer
- Ninja
- Qt 6.8 or newer, **MSVC 64-bit** build
- vcpkg
- LilyPond for Windows

The repository already includes:

- muParser
- pugixml

Install Git
-----------

Using winget:

```cmd
winget install --id Git.Git -e
```

Git for Windows usually provides Unix-style tools such as `rm` and `mv` in:

```text
C:\Program Files\Git\usr\bin
```

Add this directory to User PATH later.

Install Visual Studio C++ Tools
-------------------------------

Install Visual Studio Community or Visual Studio Build Tools.

Select:

```text
Desktop development with C++
```

Make sure these components are installed:

- MSVC x64/x86 build tools
- Windows SDK
- C++ CMake tools for Windows

Open **x64 Native Tools Command Prompt for VS** and check:

```cmd
cl
```

You should see Microsoft C/C++ compiler information.

Install Qt
----------

Install Qt using the Qt Online Installer.

Select a Qt version with the MSVC 64-bit component, for example:

```text
Qt 6.x -> MSVC 2022 64-bit
```

Set `QT_ROOT` to your Qt MSVC 64-bit directory:

```cmd
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
```

If your Qt is installed somewhere else, change the path.

Install vcpkg Dependencies
--------------------------

Choose where to install vcpkg. This guide assumes `C:\dev\vcpkg`:

```cmd
cd /d C:\dev
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

Set `VCPKG_ROOT`:

```cmd
set "VCPKG_ROOT=C:\dev\vcpkg"
```

Install required packages:

```cmd
cd /d "%VCPKG_ROOT%"
vcpkg install libsndfile:x64-windows
vcpkg install dirent:x64-windows
```

Optional: save `VCPKG_ROOT` as a user environment variable:

```cmd
setx VCPKG_ROOT "%VCPKG_ROOT%"
```

Verify:

```cmd
dir "%VCPKG_ROOT%\installed\x64-windows\include\sndfile.h"
dir "%VCPKG_ROOT%\installed\x64-windows\include\dirent.h"
dir "%VCPKG_ROOT%\installed\x64-windows\lib\*sndfile*.lib"
```

Install LilyPond
----------------

Install or extract LilyPond for Windows.

Set `LILYPOND_BIN` to the directory containing `lilypond.exe`:

```cmd
set "LILYPOND_BIN=C:\Program Files\LilyPond\usr\bin"
```

If LilyPond is installed somewhere else, change the path.

Test:

```cmd
"%LILYPOND_BIN%\lilypond.exe" --version
```

Update PATH
-----------

Add these directories to your **User PATH**:

```text
%QT_ROOT%\bin
%LILYPOND_BIN%
C:\Program Files\Git\usr\bin
```

You can edit PATH from:

```text
System Properties -> Environment Variables -> User variables -> Path -> Edit
```

After editing PATH, close and reopen all terminals.

Verify:

```cmd
where qmake
where lilypond
where rm
where mv
lilypond --version
```

Clone DISSCO
------------

Choose where to put the source code. This guide assumes `C:\dev\DISSCO-2.2.0`:

```cmd
cd /d C:\dev
git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git
cd DISSCO-2.2.0
```

Set `DISSCO_ROOT`:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
```

If the repository uses submodules, run:

```cmd
git submodule update --init --recursive
```

Configure with CMake
--------------------

Open **x64 Native Tools Command Prompt for VS**.

Set your paths. Change them if your installation paths are different:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
set "VCPKG_ROOT=C:\dev\vcpkg"
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
```

Go to the source directory:

```cmd
cd /d "%DISSCO_ROOT%"
```

Remove any old build directory:

```cmd
rmdir /s /q build
```

Configure:

```cmd
cmake -S . -B build -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DQt6_DIR="%QT_ROOT%/lib/cmake/Qt6" ^
  -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
  -DBUILD_SHARED_LIBS=OFF ^
  -DCMAKE_CXX_FLAGS="/EHsc /DNOMINMAX /DNOGDI /DMUPARSER_STATIC /D_CRT_SECURE_NO_WARNINGS" ^
  -DSNDFILE_INCLUDE_DIR="%VCPKG_ROOT%/installed/x64-windows/include" ^
  -DSNDFILE_LIBRARY="%VCPKG_ROOT%/installed/x64-windows/lib/sndfile.lib"
```

Notes:

- Qt, vcpkg packages, and MSVC must all be 64-bit.
- `MUPARSER_STATIC` is required for static muParser builds on MSVC.
- `NOMINMAX` avoids Windows `min` and `max` macro conflicts.
- `NOGDI` avoids Windows GDI name conflicts.
- `_CRT_SECURE_NO_WARNINGS` suppresses MSVC warnings for older C library calls.
- `/EHsc` enables standard C++ exception handling.

Build
-----

Build:

```cmd
cmake --build build --parallel
```

A successful build should produce:

```text
%DISSCO_ROOT%\build\CMOD\CMOD.exe
%DISSCO_ROOT%\build\LASSIE\LASSIE.exe
```

Verify:

```cmd
dir "%DISSCO_ROOT%\build\CMOD\CMOD.exe"
dir "%DISSCO_ROOT%\build\LASSIE\LASSIE.exe"
```

Run LASSIE
----------

Run:

```cmd
"%DISSCO_ROOT%\build\LASSIE\LASSIE.exe"
```

If LASSIE opens, the GUI build is working.

If Windows reports missing Qt DLLs, run:

```cmd
"%QT_ROOT%\bin\windeployqt.exe" "%DISSCO_ROOT%\build\LASSIE\LASSIE.exe"
```

Then run LASSIE again.

Run CMOD Manually
-----------------

CMOD can be run directly with a `.dissco` file:

```cmd
"%DISSCO_ROOT%\build\CMOD\CMOD.exe" "<path-to-project>\your_file.dissco"
```

Generate Score Output
---------------------

Before generating score output, make sure the project folder has a `ScoreFiles` directory:

```cmd
mkdir "<path-to-project>\ScoreFiles"
```

Then open the `.dissco` project in LASSIE and run score output.

If LilyPond is correctly installed and PATH is set, CMOD should be able to generate score files.

Cleaning and Rebuilding
-----------------------

Clean and rebuild:

```cmd
cd /d "%DISSCO_ROOT%"
cmake --build build --clean-first
```

Fully reset the build directory:

```cmd
cd /d "%DISSCO_ROOT%"
rmdir /s /q build
```

Then rerun the CMake configure command and build again.

Create a Portable Package
-------------------------

After configuring the `build` directory once, double-click:

```text
Make-Portable-for-Windows.bat
```

The script automatically:

- loads the x64 Visual Studio build environment
- finds CMake and Qt from `build/CMakeCache.txt`
- finds LilyPond from `PATH`, a previous portable package, or its standard
  installation directory
- builds the current Release binaries and runs the tests
- bundles Qt, CMOD, LilyPond, libsndfile, and app-local MSVC runtime DLLs
- validates the package with developer tool paths removed
- creates `dist/DISSCO-Windows-x64/` and
  `dist/DISSCO-Windows-x64.zip`

The target computer only needs 64-bit Windows 10 or Windows 11. It does not
need Qt, LilyPond, Visual Studio, the Visual C++ Redistributable, or
administrator access. Extract the complete ZIP and double-click `LASSIE.exe`;
`Run-DISSCO.bat` remains available as a compatibility launcher.

For non-default paths or automation, run the PowerShell entry point directly:

```powershell
.\make-portable.ps1 `
  -BuildDirectory build `
  -QtBin "C:\Qt\6.11.1\msvc2022_64\bin" `
  -LilyPondRoot "C:\Program Files\LilyPond"
```

Use `Get-Help .\Make-Portable-for-Windows.ps1 -Detailed` to view the accepted switches.
