# Build and install LASSIE on Windows

This guide builds **LASSIE**, the graphical editor, and **CMOD**, its composition
and sound engine, from the `main` branch of
[cmp-illinois/DISSCO](https://github.com/cmp-illinois/DISSCO). It then creates a portable
application: a complete folder you can copy to another computer and run.

**Only want the released application?** Follow [DOWNLOAD.md](DOWNLOAD.md).
A release may be older than `main`. If someone gives you the portable ZIP made
by this guide, skip to [Install LASSIE](#7-install-lassie). You do not need build
tools to use that ZIP.

## Which computers does this cover?

| Computer | Scope |
| --- | --- |
| Windows 11, Intel/AMD x64 | Recommended for building and running. |
| Windows 10 version 1809 or later, Intel/AMD x64 | Runtime compatibility target; the complete package needs a separate test on that Windows version. |
| Windows on Arm, 32-bit Windows, Windows 7/8/8.1 | Not covered by this guide or this x64 package. |

Check **Settings > System > About > System type**: you want an **x64-based
processor** and a **64-bit operating system**. ARM64 is different.

Qt 6.8 and 6.11 list Windows 10 **1809+** and Windows 11 as supported x64 platforms.
That dependency requirement does not certify the complete DISSCO package.
Do not assume future Qt versions have the same requirements.
[Qt 6.8 support](https://doc.qt.io/qt-6.8/supported-platforms.html#windows),
[Qt 6.11 support](https://doc.qt.io/qt-6.11/supported-platforms.html#windows).

Use an updated Windows 11 computer for a new build setup. Windows 10's ordinary
Home/Pro support ended in October 2025; ESU and LTSC have separate lifecycles.
[Microsoft release information](https://learn.microsoft.com/en-us/windows/release-health/release-information).
School or company restrictions still apply; ask the administrator rather than
disabling security controls.

## 1. Install the tools once

Skip tools you already have. Allow several gigabytes for downloads and build
files, in addition to the space requested by each installer.

| Tool | What to choose |
| --- | --- |
| [Git for Windows](https://git-scm.com/downloads/win) | Use the normal installer and allow Git to be used from the command line. |
| [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio) or Visual Studio Community | Select **Desktop development with C++**, including **MSVC x64/x86 build tools**, a **Windows SDK**, and **C++ CMake tools for Windows**. The full editor is optional. |
| [Qt Online Installer](https://www.qt.io/development/download-open-source) | Choose the **MSVC 2022 64-bit** desktop component. **Qt 6.8.1 or newer is required**; 6.8.0 has a Windows font-matching bug. The tested version is **Qt 6.11.1**. Do not choose MinGW or ARM64. |
| [LilyPond for Windows](https://lilypond.org/download.html) | Extract the **entire** Windows x86_64 ZIP. The tested version is **2.26.0**. Keep its `bin`, `lib`, and `share` folders together. |

The Visual Studio CMake component supplies CMake and Ninja. This project needs
**CMake 3.25 or later**; check the version in step 2 before installing another copy.
[Microsoft C++ workload components](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-build-tools?view=vs-2022#desktop-development-with-c).

Qt lists MSVC 2022. Visual Studio 2026 also passed the local build with Qt's
`msvc2022_64` kit. If you have several Visual Studio installations, use matching
x64 tools consistently: the final linker and packaged runtime must be at least
as new as the tools that built the dependencies. vcpkg normally selects the newest
installed Visual Studio.
[Microsoft binary compatibility](https://learn.microsoft.com/en-us/cpp/porting/binary-compat-2015-2017),
[vcpkg toolset selection](https://learn.microsoft.com/en-us/vcpkg/users/platforms/windows#selecting-a-msvc-toolset).

LilyPond's complete Windows ZIP does not need separate Python, Guile, or
Ghostscript installations.
[LilyPond setup](https://lilypond.org/doc/v2.26/Documentation/learning/command-line-setup).

## 2. Open the right terminal and set four paths

Search the Start menu for **x64 Native Tools Command Prompt for VS** and open it.
Use that window for all commands below. Administrator mode is not needed for
building in a folder you own.

**These are CMD commands, not PowerShell commands.** If the prompt starts with
`PS`, open the command prompt named above instead. Paste each complete block,
press Enter, and wait for it to finish. Stop if a command reports an error.

~~~cmd
echo %VSCMD_ARG_TGT_ARCH%
where git
where cl
cmake --version
ninja --version
~~~

The first command should print `x64`; the others should show tool locations or
versions. If a tool is missing, check the components in step 1 and reopen the
x64 command prompt.

Edit these four paths to match your computer, then paste the block:

~~~cmd
set "DISSCO_ROOT=C:\dev\DISSCO"
set "VCPKG_ROOT=C:\dev\vcpkg"
set "QT_ROOT=C:\Qt\6.11.1\msvc2022_64"
set "LILYPOND_ROOT=C:\Tools\lilypond-2.26.0"
set "PATH=%QT_ROOT%\bin;%LILYPOND_ROOT%\bin;%PATH%"
~~~

DISSCO_ROOT is the source folder. VCPKG_ROOT is a separate folder for the C++
dependency manager. QT_ROOT must contain `bin\qmake.exe`. LILYPOND_ROOT must
contain `bin\lilypond.exe`; check for an extra outer folder after extracting the
ZIP. Short paths such as `C:\dev` or `D:\dev` help avoid Windows path-length limits.

These settings affect only this terminal. **Repeat this step after reopening
it.** You do not need permanent PATH edits or Git's Unix tools (`rm` and `mv`).

~~~cmd
"%QT_ROOT%\bin\qmake.exe" -query QT_VERSION
"%LILYPOND_ROOT%\bin\lilypond.exe" --version
~~~

Expect a Qt version and `GNU LilyPond`. Fix any missing-file error before continuing.

## 3. Get the source

For a **new, empty source folder**:

~~~cmd
if not exist "%DISSCO_ROOT%" mkdir "%DISSCO_ROOT%"
git clone --depth 1 --branch main --single-branch https://github.com/cmp-illinois/DISSCO.git "%DISSCO_ROOT%"
~~~

The explicit destination avoids confusion about the downloaded folder's name.
The shallow clone omits old history to reduce the download.

**Already have the repository?** Skip cloning. Do not delete an existing folder
just to make the clone command work. Check the remote and branch:

~~~cmd
cd /d "%DISSCO_ROOT%"
git remote -v
git branch --show-current
~~~

The intended remote is `https://github.com/cmp-illinois/DISSCO.git` and the branch is
`main`. muParser and pugixml are included; no submodule setup is needed.

## 4. Install the C++ libraries

For a **new vcpkg folder**:

~~~cmd
if not exist "%VCPKG_ROOT%" mkdir "%VCPKG_ROOT%"
git clone --depth 1 https://github.com/microsoft/vcpkg.git "%VCPKG_ROOT%"
call "%VCPKG_ROOT%\bootstrap-vcpkg.bat"
~~~

If vcpkg is already installed, skip that block. Then run:

~~~cmd
"%VCPKG_ROOT%\vcpkg.exe" install libsndfile:x64-windows dirent:x64-windows
~~~

Wait for successful completion; the first installation may take a while.
Already installed packages are reused. Keep `x64-windows` so the libraries
match the compiler and Qt.
[vcpkg setup](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started).

## 5. Configure, build, and test

Keep using the same x64 command prompt. Paste this **whole block**. Each `^`
continues the command on the next line; do not add spaces after it.

~~~cmd
cmake -S "%DISSCO_ROOT%" -B "%DISSCO_ROOT%\build" -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-windows ^
  -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
  -DBUILD_SHARED_LIBS=OFF ^
  -DBUILD_TESTING=ON ^
  -DCMAKE_CXX_FLAGS="/EHsc /DNOMINMAX /DNOGDI /D_CRT_SECURE_NO_WARNINGS"
~~~

Wait for **Configuring done** and **Generating done**. CMake finds libsndfile
through vcpkg and Qt through QT_ROOT; you do not need paths to individual `.lib`
files. The repository sets muParser's static-library definition itself.

~~~cmd
cmake --build "%DISSCO_ROOT%\build" --parallel 2
ctest --test-dir "%DISSCO_ROOT%\build" --output-on-failure --no-tests=error
~~~

Two parallel jobs keep memory use modest. If memory runs out, retry with
`--parallel 1`. Warnings are not necessarily build failures; check the final
result. The test command should report **100% tests passed**.

The build creates `build\LASSIE\lassie.exe` and `build\CMOD\cmod.exe`. Do not copy
either EXE alone to install it: the application still needs supporting files.

## 6. Make the complete portable application

~~~cmd
cd /d "%DISSCO_ROOT%"
Make-Portable-for-Windows.bat -LilyPondRoot "%LILYPOND_ROOT%"
~~~

The script loads the build environment, checks the build and tests, bundles
Qt, CMOD, audio DLLs, the Microsoft C++ runtime, and LilyPond, and checks startup
with developer-tool paths removed. Wait for **Portable package created
successfully**. Press a key when the batch window asks.

~~~text
dist\DISSCO-Windows-x64\             Complete application folder
dist\DISSCO-Windows-x64.zip          Distribution ZIP
dist\DISSCO-Windows-x64.zip.sha256   ZIP checksum
~~~

Double-clicking `Make-Portable-for-Windows.bat` also works if LilyPond is on PATH
or in a standard installation location. The command above handles custom paths.
For an explicitly selected Qt installation, add `-QtBin "%QT_ROOT%\bin"` to it.
Do not use `-SkipTests` or `-SkipSmokeTest` for a package you give to someone else.

The portable folder contains permitted Release C++ runtime DLLs, so a separate
Visual C++ Redistributable installation is not normally needed. Maintainers must
keep those bundled DLLs updated.
[Microsoft application-local deployment](https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method).

## 7. Install LASSIE

1. If you received the ZIP, right-click it and choose **Extract All**. Do not run
   the application from inside the ZIP.
2. Paste `%LOCALAPPDATA%\Programs` into File Explorer's address bar. Create that
   folder if necessary. Copy the **whole** `DISSCO-Windows-x64` folder there.
   Use an empty destination; do not mix files from different versions.
3. Double-click **lassie.exe** in the copied folder. Keep all DLLs and subfolders
   with it. Administrator access and permanent PATH changes are not normally needed.
4. To add a desktop shortcut, right-click `lassie.exe` and choose **Show more
   options > Send to > Desktop (create shortcut)**. Older Windows versions show
   **Send to** directly.

The usual installed path is
`%LOCALAPPDATA%\Programs\DISSCO-Windows-x64\lassie.exe`. This portable installation
does not replace another DISSCO installer or change `.dissco` file associations.
Open projects from the new LASSIE window to be sure you are using this copy.

Check the installed copy from File Explorer, **not just the developer terminal**:

- Create a project in a separate writable folder, such as
  `Documents\DISSCO Projects\InstallCheck`. In **Project Properties**, enter
  `1` for **Piece Duration**, then click **OK**. Save, close LASSIE, and reopen
  the project from **Recent Projects**. Confirm the duration is still `1`.
- Follow the [sound tutorial](Documents/tutorial_sound.md) or
  [score tutorial](Documents/tutorial_score.md), starting at the LASSIE steps,
  for a first composition. An empty project is not yet playable.
- Sound and score output go into `SoundFiles` and `ScoreFiles` beside the project.
  CMOD creates these folders automatically. The portable application uses its
  bundled LilyPond for PDFs.

Keep projects outside the application and source folders. If Windows blocks
execution, verify the source and checksum and follow your device's security policy.

## Updating and cleaning up

For an unchanged checkout of the intended `main` branch, reopen the x64 command
prompt, repeat the four-path setup, and run:

~~~cmd
cd /d "%DISSCO_ROOT%"
git pull --ff-only
~~~

If Git reports local changes or a diverged branch, stop and decide what to keep.
Repeat steps 5 and 6, close the old LASSIE, and test a new portable folder before
removing the old copy.

After checking the installed copy:

- Delete temporary test projects and their generated output.
- You may delete **build** and **dist** in the source folder using File Explorer.
  Confirm you are in DISSCO_ROOT first. Keep the installed application and real projects.
- Keep the source, documentation, and `.git` folder for future updates. Keep the
  ZIP and checksum only if you need a distribution copy.

Changing the compiler, Qt kit, or architecture requires a fresh **build** folder.
Ordinary edits can reuse it. Do not delete source or project files to reset a build.

## Common problems

| What you see | What to do |
| --- | --- |
| `cmake`, `ninja`, or `cl` not found | Reopen the x64 Native Tools Command Prompt and check the Visual Studio components. |
| Qt not found | Check QT_ROOT and its `lib\cmake\Qt6` folder. Choose the MSVC x64 kit, not MinGW. |
| Generator/toolchain mismatch | Remove only the old **build** folder and configure again. |
| Missing DLL or Qt platform plugin | Use the complete portable folder, not an EXE copied out of it. Recreate the package if files are missing. |
| `Get-FileHash` or `Compress-Archive` missing when the batch file is started from PowerShell 7 | Use the current batch file. It resets only its child's module search path. No global execution-policy change is needed. |
| LilyPond missing or PDFs fail | Pass the actual LilyPond root in step 6 and retain the complete archive, including fonts. Read the CMOD output for the specific error. |

The PowerShell issue is explained in
[Microsoft's module-path documentation](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_psmodulepath#starting-windows-powershell-from-powershell-7).

## Verification record — 2026-08-27

Tested on Windows 11 x64 (build 26200), Visual Studio 2026 18.7.3 / MSVC 19.51,
CMake 4.3.1, Ninja 1.13.2, Qt 6.11.1, and LilyPond 2.26.0. Git, Visual Studio,
Qt, LilyPond, and the vcpkg libraries were already installed; their first-time
installer screens were not tested on a clean Windows machine.

- A fresh GitHub clone in a path containing spaces configured and built with
  the commands above; the repository's one CTest test passed.
- The batch entry point was tested from PowerShell 7, including an explicit
  Qt path and a build environment without Qt/LilyPond on its inherited PATH.
- The ZIP checksum and all 1,272 extracted files matched the package. Installed
  LASSIE opened and saved/reopened a project with developer paths removed.
  Installed CMOD displayed help, and bundled LilyPond generated a one-page PDF.

These checks cover installation and startup, not every composition feature.
Windows 10, ARM64, and a clean virtual machine were not tested.
