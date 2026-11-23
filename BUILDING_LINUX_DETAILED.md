# Building DISSCO on Linux (detailed guide)

This guide takes you from installing the required tools to opening LASSIE,
explains each step, and covers updating, troubleshooting, and packaging. If you
only need the commands, use the [quick commands](BUILDING_LINUX.md) page.

**Only want to use DISSCO?** Download the released AppImage from
[DOWNLOAD.md](DOWNLOAD.md#linux). Building from the current source lets you use
fixes that have not yet appeared in a release.

You will build:

- `lassie`, the graphical editor
- `cmod`, the composition and sound-generation engine used by LASSIE

You do not need Qt Creator, and you do not need to build Qt yourself.

Run the commands in this guide in a Terminal window. To open one, search for
**Terminal** in your applications menu. Copy one code block at a time and paste
it with **Ctrl+Shift+V** or right-click > **Paste** (plain Ctrl+V does not paste
in Terminal). Press **Enter** and wait for the prompt to come back before the
next block.

## Which Linux systems does this cover?

Check your system and computer type:

```sh
cat /etc/os-release
uname -m
```

DISSCO needs Qt 6.8 or newer. Whether your Linux provides it decides which
steps you follow:

| System | Qt from `apt` | Follow |
| --- | --- | --- |
| Debian 13 (`trixie`), LMDE 7 | 6.8.2 | [Debian 13 and Ubuntu 26.04](#debian-13-and-ubuntu-2604-qt-from-apt) |
| Ubuntu 26.04 LTS or newer | 6.10.2 or newer | [Debian 13 and Ubuntu 26.04](#debian-13-and-ubuntu-2604-qt-from-apt) |
| Ubuntu 24.04 LTS, Linux Mint 22, Pop!_OS 24.04 | 6.4.2, too old | [Ubuntu 22.04 and 24.04](#ubuntu-2204-and-2404-download-qt) |
| Ubuntu 22.04 LTS, Linux Mint 21, Pop!_OS 22.04 | 6.2.4, too old | [Ubuntu 22.04 and 24.04](#ubuntu-2204-and-2404-download-qt) |
| Anything else, including Debian 12 and ARM computers | varies | [Other Linux systems](#other-linux-systems) |

Linux Mint and Pop!_OS use the Ubuntu release shown in the same row. The
Ubuntu 22.04 and 24.04 steps download Qt for `x86_64` only; `uname -m` must
print `x86_64` for them.

## What do I need to install?

| Requirement | Debian 13 and Ubuntu 26.04 | Ubuntu 22.04 and 24.04 |
| --- | --- | --- |
| Git | `apt` | `apt` |
| C and C++20 compilers | GCC from `build-essential` | GCC from `build-essential` |
| CMake 3.25 or newer | `apt` | Downloaded into your home folder (Ubuntu 22.04's `apt` CMake is 3.22) |
| libsndfile development files | `libsndfile1-dev` from `apt` | `libsndfile1-dev` from `apt` |
| Qt 6.8 or newer (Widgets, Core, Xml, Network) | `qt6-base-dev` from `apt` | Qt 6.11.2 downloaded into your home folder |
| Linux graphics libraries | Installed with `qt6-base-dev` | `apt`, listed in the package step |
| muParser and pugixml | Included in the DISSCO repository | Included in the DISSCO repository |
| LilyPond (optional, for scores and PDFs) | `apt` | `apt` |

**Why not just run `sudo apt install qt6.8`?** Qt version numbers are not
package names. The package is called `qt6-base-dev`, and its version depends on
your Linux release: Ubuntu 22.04 provides Qt 6.2 and Ubuntu 24.04 provides
Qt 6.4, both older than DISSCO's minimum. On those systems this guide downloads
Qt 6.11.2, the version the Linux build and release workflows use.

## Install the packages (once, needs sudo)

This is the only step that needs `sudo`. On your own computer, run it yourself.
On lab or shared computers, students usually cannot use `sudo`; ask the
administrator to run it once rather than trying to work around school or
company restrictions. Every later step runs in your home folder without `sudo`.

**Debian 13 and Ubuntu 26.04:**

```sh
sudo apt update
sudo apt install -y git build-essential cmake qt6-base-dev libsndfile1-dev lilypond
```

**Ubuntu 22.04 and 24.04:**

```sh
sudo apt update
sudo apt install -y git build-essential ninja-build python3-venv libsndfile1-dev \
  libgl1-mesa-dev libxkbcommon-x11-0 libxcb-cursor0 \
  libxkbcommon-dev '^libxcb.*-dev' libx11-xcb-dev \
  libglu1-mesa-dev libxrender-dev libxi-dev libxkbfile-dev lilypond
```

`lilypond` is only needed for score and PDF output and can be left out.
`python3-venv` is required on Ubuntu even though Python is already installed:
without it, the Ubuntu steps cannot create their tools folder.

Type your password if asked. Terminal does not show characters while you type a
password; this is normal. Packages that are already installed are kept.

On your own Debian computer, `sudo` may not work even though you installed the
system: Debian does not give your account `sudo` if you set a root password
during installation. In that case, instead of the two Debian 13 commands, run
this line and type the root password when asked. It returns to your own account
when it finishes:

```sh
su -l -c 'apt update && apt install -y git build-essential cmake qt6-base-dev libsndfile1-dev lilypond'
```

## Debian 13 and Ubuntu 26.04: Qt from apt

### 1. Check the packages

```sh
dpkg -s git build-essential cmake qt6-base-dev libsndfile1-dev > /dev/null && echo ready
qmake6 -query QT_VERSION
```

The first command must print `ready`, and the second must print 6.8 or newer.
If the first command reports that a package `is not installed`, the
[package step](#install-the-packages-once-needs-sudo) has not been done on this
computer.

### 2. Download DISSCO

```sh
cd
git clone https://github.com/cmp-illinois/DISSCO.git
cd DISSCO
```

`cd` on its own goes to your home folder. This creates a `DISSCO` folder there
and opens it in Terminal.

If Git says `destination path 'DISSCO' already exists`, you downloaded DISSCO
before. Run `cd ~/DISSCO`, get the latest source as described in
[Updating after new fixes](#updating-after-new-fixes), and continue with step 3.

### 3. Configure

From the `DISSCO` folder, run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

This writes the build files into a `build` folder. CMake finds the Qt that
`apt` installed on its own, so no Qt path is needed. `Release` turns on the
compiler's optimizations, which makes sound generation faster. A successful
run ends with `Configuring done`, `Generating done`, and
`Build files have been written to`.

### 4. Build, test, and open LASSIE

```sh
cmake --build build --parallel 2
```

This takes several minutes. It has worked when one of the last lines says
`[100%] Built target` and Terminal is back at the prompt. (A line that mentions
`muParserError.cpp` is only a file name.) If the build fails, it stops with a
line ending in `Error 2`; scroll up to the first line that contains `error:` and
look that up in [Troubleshooting](#troubleshooting). Two build jobs keep memory
use modest; on a computer with plenty of memory you can use a larger number.

Run the included checks:

```sh
ctest --test-dir build --output-on-failure
```

Look for `100% tests passed`. These automatic checks open LASSIE windows
without showing them; they do not test sound output, so also try your own
project.

Open LASSIE:

```sh
./build/LASSIE/lassie
```

Choose **File > Open Project** to open a `.dissco` file, then **Project > Run**
to generate sound. Keep this Terminal window open while you use LASSIE, so you
can see any error messages. The engine LASSIE uses is `build/CMOD/cmod`; you do
not need a separate install step.

## Ubuntu 22.04 and 24.04: download Qt

These steps put Qt 6.11.2 and a recent CMake in your home folder, so they do
not change the system or need `sudo`.

**First**, the Ubuntu 22.04 and 24.04 packages from
[Install the packages](#install-the-packages-once-needs-sudo) must be
installed. On your own computer, run those two commands yourself; on a lab
computer, ask the administrator. Without them, step 1 below fails with an
`ensurepip is not available` message.

If your Terminal prompt starts with `(base)` because you use Conda, run
`conda deactivate` first.

### 1. Install Qt 6.11.2 and CMake into your home folder

Run these three commands once:

```sh
python3 -m venv ~/.local/share/dissco-tools
~/.local/share/dissco-tools/bin/python -m pip install 'aqtinstall==3.3.*' 'cmake>=3.25,<4'
~/.local/share/dissco-tools/bin/python -m aqt install-qt linux desktop 6.11.2 linux_gcc_64 --outputdir ~/Qt
```

The first command creates a separate tools folder, so the system's Python stays
unchanged. The second installs CMake and the Qt download tool
[aqtinstall](https://aqtinstall.readthedocs.io/en/stable/getting_started.html)
into it. The third downloads Qt from the official Qt download server into
`~/Qt/6.11.2/gcc_64`. It downloads about 200 MB and needs about 1.6 GB of space
in your home folder; check your disk quota on shared lab computers. Here `~`
means your home folder.

The commands in this section start CMake and Python by their full path, such as
`~/.local/share/dissco-tools/bin/cmake`. Type them exactly as shown: on Ubuntu,
plain `cmake` is either not installed or, on 22.04, too old.

Check the result:

```sh
~/Qt/6.11.2/gcc_64/bin/qmake -query QT_VERSION
~/.local/share/dissco-tools/bin/cmake --version
```

The first command must print `6.11.2`, and the second must report CMake 3.25
or newer. You do not need to remove Ubuntu's own Qt packages.

### 2. Download DISSCO

```sh
cd
git clone https://github.com/cmp-illinois/DISSCO.git
cd DISSCO
```

If Git says `destination path 'DISSCO' already exists`, you downloaded DISSCO
before. Run `cd ~/DISSCO`, get the latest source as described in
[Updating after new fixes](#updating-after-new-fixes), and continue with step 3.

### 3. Configure

From the `DISSCO` folder, run:

```sh
~/.local/share/dissco-tools/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOME/Qt/6.11.2/gcc_64"
```

`CMAKE_PREFIX_PATH` tells CMake to use the Qt from step 1 instead of Ubuntu's
older one. `Release` turns on the compiler's optimizations, which makes sound
generation faster. A successful run ends with `Configuring done`,
`Generating done`, and `Build files have been written to`. A
`Could NOT find WrapVulkanHeaders` message along the way is harmless.

### 4. Build, test, and open LASSIE

```sh
~/.local/share/dissco-tools/bin/cmake --build build --parallel 2
```

This takes several minutes. It has worked when one of the last lines says
`[100%] Built target` and Terminal is back at the prompt. (A line that mentions
`muParserError.cpp` is only a file name.) If the build fails, it stops with a
line ending in `Error 2`; scroll up to the first line that contains `error:` and
look that up in [Troubleshooting](#troubleshooting).

Run the included checks:

```sh
~/.local/share/dissco-tools/bin/ctest --test-dir build --output-on-failure
```

Look for `100% tests passed`. These automatic checks open LASSIE windows
without showing them; they do not test sound output, so also try your own
project.

Open LASSIE:

```sh
./build/LASSIE/lassie
```

Choose **File > Open Project** to open a `.dissco` file, then **Project > Run**
to generate sound. Keep this Terminal window open while you use LASSIE, so you
can see any error messages.

## Opening LASSIE next time

```sh
~/DISSCO/build/LASSIE/lassie
```

If your DISSCO folder is somewhere else, use that path instead. You do not need
to repeat any setup. Keep the `DISSCO/build` folder (and, on Ubuntu 22.04 or
24.04, the `~/Qt` folder) where they are: LASSIE runs the `cmod` engine from the
`build` folder, and the Ubuntu build loads Qt from `~/Qt`.

If you moved or renamed the DISSCO folder, or deleted `build`, go into the
DISSCO folder, run `rm -rf build`, and repeat the configure and build steps for
your system.

## Updating after new fixes

Close LASSIE first. Then check whether you have changed any files in the
DISSCO folder:

```sh
cd ~/DISSCO
git status --short
```

If this lists files, keep a copy of your work before updating. If you are
unsure what the listed files are, stop and ask for help. Then get the new
source:

```sh
git switch main
git pull --ff-only
```

An update succeeds when Git reports a fast-forward or `Already up to date`.
Then repeat the configure and build steps for your system (steps 3 and 4 of
your section). Running the configure step again is safe, and it is needed if an
earlier attempt never finished. Pulling only updates the source files;
rebuilding is what updates the program you run.

If you change Qt or the compiler, add `--fresh` after `cmake` in the configure
command once; it clears the old settings.

## Troubleshooting

If a command printed many lines, look for the first error, not the last line.

| What you see | What to do |
| --- | --- |
| `is not in the sudoers file`, `sudo: command not found`, or `sudo` asks for a password you do not have | On a lab or shared computer, you cannot install packages yourself; ask the administrator to run the [package step](#install-the-packages-once-needs-sudo), then continue. On your own Debian computer, use the `su -l -c` line at the end of that step. |
| `Unable to locate package qt6.8` or `qt8` | These are not package names. Use the package lists in the [package step](#install-the-packages-once-needs-sudo). |
| `git: command not found`, or `Command 'git' not found` | The [package step](#install-the-packages-once-needs-sudo) has not been done on this computer. |
| `No CMAKE_CXX_COMPILER could be found`, or `CMAKE_MAKE_PROGRAM is not set` | The compilers from `build-essential` are missing; see the [package step](#install-the-packages-once-needs-sudo). Then repeat the configure command with `--fresh` added after `cmake`. |
| `cmake: command not found`, or `Command 'cmake' not found, but can be installed with` | Debian 13 and Ubuntu 26.04: the package step has not been done. Ubuntu 22.04 and 24.04: do not install the `snap` or `apt` CMake that Ubuntu suggests. Type the full path `~/.local/share/dissco-tools/bin/cmake`; if that file does not exist, repeat step 1 of the Ubuntu steps. |
| `The virtual environment was not created successfully` (it mentions `ensurepip`), `No module named pip`, or `No module named aqt` | Scroll up to the first error. If it mentions `ensurepip` or `python3-venv`, the Ubuntu package step has not been done: run the whole Ubuntu command from the [package step](#install-the-packages-once-needs-sudo) (or ask the administrator), then run `rm -rf ~/.local/share/dissco-tools` and repeat step 1 of the Ubuntu steps. Otherwise, check the internet connection and repeat step 1. |
| pip reports `externally-managed-environment` | The command used the system's Python. Use the full path `~/.local/share/dissco-tools/bin/python` exactly as shown. Do not use `sudo pip` or `--break-system-packages`. |
| `CMake 3.25 or higher is required`, followed by `You are running version 3.22.1` | Ubuntu 22.04's own CMake ran. Use `~/.local/share/dissco-tools/bin/cmake` instead of `cmake`. |
| `Could not find a package configuration file provided by "Qt6"` | CMake found no Qt at all. Debian 13 and Ubuntu 26.04: `qt6-base-dev` is not installed; see the package step. Ubuntu 22.04 and 24.04: check that `~/Qt/6.11.2/gcc_64/bin/qmake -query QT_VERSION` prints `6.11.2` (if not, repeat step 1), then repeat the configure command exactly as shown, with `--fresh` added after `cmake`. |
| `Could not find a configuration file for package "Qt6" that is compatible with requested version "6.8"` | CMake found a Qt that is too old. On Ubuntu 22.04 or 24.04, follow the [Ubuntu steps](#ubuntu-2204-and-2404-download-qt); if you already did, repeat their configure command with `--fresh` added after `cmake`. Do not edit DISSCO's `CMakeLists.txt`. |
| `Could NOT find SndFile` | `libsndfile1-dev` is not installed; see the [package step](#install-the-packages-once-needs-sudo). Then repeat the configure command. |
| `The source directory ... does not appear to contain CMakeLists.txt` | CMake was started outside the DISSCO folder, or with `cmake ..`. Run `cd ~/DISSCO`, then run the configure command from step 3 of your section exactly as shown. It contains `-S . -B build`; on Ubuntu 22.04 and 24.04 it starts with `~/.local/share/dissco-tools/bin/cmake`. |
| `build is not a directory`, `could not load cache`, `not a CMake build directory`, `./build/LASSIE/lassie: No such file or directory`, or checks marked `Not Run` | Terminal is not in the DISSCO folder (for example, in a new Terminal window), or the build has not finished. Run `cd ~/DISSCO`, then the configure and build steps for your system. If the build stops, look up its first `error:` line. |
| `Target "LASSIE" links to: DISSCO::ModifierUsage but the target was not found`, or `Could not find a package configuration file provided by "SndFile"` | CMake was run inside the `LASSIE` or `CMOD` folder. Run `cd ~/DISSCO` and `rm -rf LASSIE/build CMOD/build`, then run the configure command from the DISSCO folder. |
| `In-source builds are prohibited` | `cmake` was run without `-B build`. In the DISSCO folder, delete `CMakeCache.txt` and the `CMakeFiles` folder that it created, then use the configure command exactly as shown. |
| The compiler stops with `Killed`, or the computer runs out of memory | Build with one job: replace `--parallel 2` with `--parallel 1`. If it still fails, send the first compiler error, not only the last line. |
| `could not connect to display` (often followed by messages about the `xcb` plugin) | LASSIE needs a graphical desktop session; installing libraries does not fix this. Over SSH, connect with `ssh -X`, or start LASSIE on the computer itself. |
| `Could not load the Qt platform plugin "xcb"` or `xcb-cursor0 or libxcb-cursor0 is needed`, without a `could not connect to display` line | A graphics library is missing. Ask the administrator to run `sudo apt install libxcb-cursor0 libxkbcommon-x11-0`. Run `QT_DEBUG_PLUGINS=1 ./build/LASSIE/lassie` for details. |
| A warning that the `wayland` plugin could not be found, after which LASSIE opens | Harmless: Qt falls back to X11. To use Wayland directly, the administrator can install `qt6-wayland`. |
| `The current CMakeCache.txt directory ... is different than the directory ... where CMakeCache.txt was created`, or **Project > Run** fails after you moved or renamed the DISSCO folder | The `build` folder still records the old location. Run `rm -rf build` in the DISSCO folder, then repeat the configure and build steps. |
| `LilyPond failed to generate the score PDF` | Run `lilypond --version`. If the command is not found, ask the administrator to install `lilypond` and run the project again. If it prints a version, LilyPond found a problem in the generated score: send the LilyPond messages printed just above this error to the person helping you. Sound files are written either way. |
| Git reports local changes or cannot fast-forward | Stop and keep your work. Send the error and the output of `git status --short --branch` to the person helping you. Do not use `git reset --hard` or delete the folder to fix it. |

These messages can be ignored if the step still succeeds:
`Could NOT find WrapVulkanHeaders` (DISSCO does not use Vulkan), and an empty
`CMOD_BINARY set to` line during configuration.

**Asking for help:** send the command that failed, its full error output, and
the output of:

```sh
cat /etc/os-release
uname -m
cmake --version
```

On Ubuntu 22.04 or 24.04, also send the output of
`~/.local/share/dissco-tools/bin/cmake --version` and
`~/Qt/6.11.2/gcc_64/bin/qmake -query QT_VERSION`.

## Optional: score and PDF output

Making sound does not need LilyPond. Projects that print a score call the
`lilypond` program, which the administrator installs with:

```sh
sudo apt install -y lilypond
lilypond --version
```

The package lists above already include it. CMOD finds LilyPond through your
`PATH`, so a separate setting is not needed.

## Optional: developer setup

These are not needed to use DISSCO.

- **Ninja:** install `ninja-build` and add `-G Ninja` after `cmake` in the
  configure command. If `build` was already configured without Ninja, also add
  `--fresh`; otherwise CMake stops because the folder keeps its first
  generator. `cmake --preset default --fresh` also uses Ninja and the `build`
  folder, but it does not set `Release` and, on Ubuntu 22.04 and 24.04, does
  not point at `~/Qt`, so add those options yourself. On Ubuntu 22.04 and
  24.04, use the full path `~/.local/share/dissco-tools/bin/cmake` for all of
  these.
- **lld:** if `ld.lld` is installed (`sudo apt install lld`), DISSCO links with
  it automatically and the configure output shows `Using lld linker`.
- **ccache:** installing `ccache` alone has no effect. Add
  `-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache` to
  the configure command. Because LASSIE and CMOD use precompiled headers, also
  `export CCACHE_SLOPPINESS=pch_defines,time_macros`; see the
  [ccache manual](https://ccache.dev/manual/latest.html#_precompiled_headers).
- **Cleaning:** `cmake --build build --target clean` removes compiled files,
  `cmake --fresh ...` discards the configuration, and `rm -rf build` removes
  everything that was built. Your source and project files are not affected.
- **Out-of-source builds only:** the top-level `CMakeLists.txt` stops with an
  error if you configure inside the source folder; always use a separate
  `build` folder.

## Optional: build an AppImage

An AppImage bundles LASSIE, CMOD, Qt, and the audio libraries into one file
that runs on other Linux computers. You do not need one to use your own build.

First finish the configure and build steps. Then have the administrator install
the packaging tools for your system.

Ubuntu 22.04:

```sh
sudo apt install -y curl file desktop-file-utils libfuse2
```

Ubuntu 24.04, Ubuntu 26.04, and Debian 13:

```sh
sudo apt install -y curl file desktop-file-utils libfuse2t64
```

Build the package from the DISSCO folder. The `QMAKE` setting tells the
packaging tool which Qt to bundle.

Ubuntu 22.04 and 24.04:

```sh
QMAKE="$HOME/Qt/6.11.2/gcc_64/bin/qmake" ~/.local/share/dissco-tools/bin/cmake --build build --target appimage
```

Debian 13 and Ubuntu 26.04 (bundling the system Qt has not been tested):

```sh
QMAKE=/usr/lib/qt6/bin/qmake cmake --build build --target appimage
```

The first run downloads the `linuxdeploy` packaging tools from GitHub into
`build/.linuxdeploy/`, so it needs internet access. The result is
`build/DISSCO-<version>-Linux-<architecture>.AppImage`; see
[DOWNLOAD.md](DOWNLOAD.md#linux) for how to run it.

To package only the command-line engine, run
`cmake --build build --target cmod-package` (with the full path to `cmake` on
Ubuntu 22.04 and 24.04). The result,
`build/CMOD-<version>-Linux-<architecture>.AppImage`, contains CMOD and its
audio libraries, but not LASSIE or Qt. Neither package includes LilyPond.

An AppImage only runs on Linux systems at least as new as the one it was built
on. The release workflow therefore builds Linux packages on Ubuntu 22.04; an
AppImage built on Debian 13 or Ubuntu 26.04 will not run on older systems.

## Other Linux systems

DISSCO needs a C++20 compiler, CMake 3.25 or newer, the libsndfile development
files, and Qt 6.8 or newer with the Widgets, Core, Xml, and Network modules.
If your distribution provides those versions, install its development packages
and follow the [Debian 13 and Ubuntu 26.04](#debian-13-and-ubuntu-2604-qt-from-apt)
steps. On Debian-based systems, `apt-cache policy qt6-base-dev` shows which Qt
version is available, without needing `sudo`.

- **Other Ubuntu- or Debian-based systems** (for example Zorin OS or
  elementary OS): look for `UBUNTU_CODENAME` or `DEBIAN_CODENAME` in
  `/etc/os-release`. With `jammy` (22.04) or `noble` (24.04), follow the
  [Ubuntu 22.04 and 24.04 steps](#ubuntu-2204-and-2404-download-qt). With
  `trixie` (Debian 13) or `resolute` (Ubuntu 26.04), follow the
  [Debian 13 and Ubuntu 26.04 steps](#debian-13-and-ubuntu-2604-qt-from-apt).
- **Debian 12** provides Qt 6.4, which is too old. Upgrading to Debian 13 is
  the simplest fix. On an `x86_64` computer you can instead follow the
  [Ubuntu 22.04 and 24.04 steps](#ubuntu-2204-and-2404-download-qt), which use
  the same package names on Debian 12; this has not been tested.
- **ARM computers (`aarch64`):** the Qt download in the Ubuntu steps is for
  `x86_64` only. Use a distribution whose own Qt is new enough, or download
  the matching ARM Qt yourself.
- Qt's downloaded `x86_64` libraries need a reasonably recent system (glibc
  2.34 or newer); see the
  [Qt 6.11 Linux requirements](https://doc.qt.io/qt-6.11/linux.html).

## How these steps are verified

The project's GitHub Actions build workflow runs on every change to the code
or to the workflow itself:

- The **Linux build** job runs the Ubuntu 22.04 and 24.04 package command and
  Qt download commands on both Ubuntu versions. It then configures with Ninja
  and treats compiler warnings as errors, instead of using the plain configure
  command shown above.
- The **Linux build with distribution Qt** job runs the
  [quick commands](BUILDING_LINUX.md) (package install, configure, and build),
  then the included checks, in Debian 13 and Ubuntu 26.04 containers.

Not covered by these checks: classroom computers where students have no
`sudo` (the commands are the same once the package step is done),
Wayland-only desktops, ARM computers, score and PDF output, and AppImage
packaging, which only the release workflow builds.
