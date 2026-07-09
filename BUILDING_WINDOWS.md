Building on Windows
===================

This guide explains how to build and run DISSCO on Windows with MSVC, CMake, Ninja, Qt, vcpkg, libsndfile, dirent, Git for Windows, and LilyPond.

DISSCO contains three main parts:

- **LASS**: sound synthesis library
- **CMOD**: composition module
- **LASSIE**: Qt GUI for editing `.dissco` files

LASSIE can run CMOD, and CMOD can generate LilyPond score output.

---

Before You Start
----------------

Use **x64 Native Tools Command Prompt for VS** for all build commands below.

The commands in this guide use **CMD syntax**. If you use PowerShell, line continuation and environment variable syntax are different.

You may install DISSCO, vcpkg, Qt, and LilyPond anywhere. In this guide, replace the following paths with your own paths:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
set "VCPKG_ROOT=C:\dev\vcpkg"
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
set "LILYPOND_BIN=C:\Program Files\LilyPond\usr\bin"
```

These are examples only. Choose any directories you prefer, but keep the paths consistent in the commands.

---

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

The following are installed through vcpkg:

- libsndfile
- dirent

---

1. Install Git
--------------

Using winget:

```cmd
winget install --id Git.Git -e
```

Git for Windows usually provides Unix-style tools such as `rm` and `mv` in:

```text
C:\Program Files\Git\usr\bin
```

Add this directory to your User PATH later.

---

2. Install Visual Studio C++ Tools
----------------------------------

Install Visual Studio Community or Visual Studio Build Tools.

Select:

```text
Desktop development with C++
```

Make sure the following are installed:

- MSVC x64/x86 build tools
- Windows SDK
- C++ CMake tools for Windows

Open **x64 Native Tools Command Prompt for VS** and check:

```cmd
cl
```

You should see Microsoft C/C++ compiler information.

---

3. Install Qt
-------------

Install Qt using the Qt Online Installer.

Select a Qt version with the MSVC 64-bit component, for example:

```text
Qt 6.x -> MSVC 2022 64-bit
```

Set `QT_ROOT` to the Qt MSVC 64-bit directory, for example:

```cmd
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
```

The important subdirectories are:

```text
%QT_ROOT%\bin
%QT_ROOT%\lib\cmake\Qt6
```

---

4. Install vcpkg Dependencies
-----------------------------

Choose a location for vcpkg, then run:

```cmd
cd /d C:\dev
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

Set `VCPKG_ROOT` to your vcpkg directory:

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

---

5. Install LilyPond
-------------------

Install or extract LilyPond for Windows.

Set `LILYPOND_BIN` to the directory containing `lilypond.exe`, for example:

```cmd
set "LILYPOND_BIN=C:\Program Files\LilyPond\usr\bin"
```

Other possible locations include:

```text
C:\Program Files\LilyPond\bin
C:\Users\<UserName>\AppData\Local\Programs\LilyPond\bin
D:\Programs\lilypond-2.26.0\bin
```

Test:

```cmd
"%LILYPOND_BIN%\lilypond.exe" --version
```

---

6. Update PATH
--------------

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

---

7. Clone DISSCO
---------------

Choose any source directory and clone the repository:

```cmd
cd /d C:\dev
git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git
cd DISSCO-2.2.0
```

Set `DISSCO_ROOT` to the source directory:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
```

If the repository uses submodules, run:

```cmd
git submodule update --init --recursive
```

---

8. Configure with CMake
-----------------------

Open **x64 Native Tools Command Prompt for VS**.

Set or confirm your paths:

```cmd
set "DISSCO_ROOT=C:\dev\DISSCO-2.2.0"
set "VCPKG_ROOT=C:\dev\vcpkg"
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
```

Go to the DISSCO source directory:

```cmd
cd /d "%DISSCO_ROOT%"
cd
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

- Use the x64 Native Tools Command Prompt.
- Qt, vcpkg packages, and MSVC must all be 64-bit.
- `MUPARSER_STATIC` is required for static muParser builds on MSVC.
- `NOMINMAX` avoids Windows `min` and `max` macro conflicts.
- `NOGDI` avoids Windows GDI name conflicts.
- `_CRT_SECURE_NO_WARNINGS` suppresses MSVC warnings for older C library calls.
- `/EHsc` enables standard C++ exception handling.

---

9. Build
--------

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

---

10. Run LASSIE
--------------

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

---

11. Run CMOD Manually
---------------------

CMOD can be run directly with a `.dissco` file:

```cmd
"%DISSCO_ROOT%\build\CMOD\CMOD.exe" "<path-to-project>\your_file.dissco"
```

Example:

```cmd
"%DISSCO_ROOT%\build\CMOD\CMOD.exe" "C:\Users\<UserName>\Documents\DISSCO\TestProject\TestProject.dissco"
```

---

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

---
