# Building DISSCO on macOS

> **macOS release currently unavailable:** The project does not yet have the
> Apple Developer credentials required to sign and notarize a macOS release.
> Until a signed and notarized release is available, macOS users must follow
> this guide to build DISSCO from source. Releases for other platforms are
> listed in [DOWNLOAD.md](DOWNLOAD.md).

This guide covers native builds on all currently supported Macs:

- Apple silicon Macs (`arm64`)
- 64-bit Intel Macs (`x86_64`)
- macOS 14 or newer

These limits match the current [Homebrew installation requirements](https://docs.brew.sh/Installation),
[Qt 6 supported platforms](https://doc.qt.io/qt-6/supported-platforms.html),
and [MacPorts Qt 6 port](https://ports.macports.org/port/qt6/details/).
Older macOS releases may work with older dependencies, but are not covered by
this tested procedure.

The same commands work on both processor types. Homebrew chooses the correct
installation directory automatically, so do not replace `brew --prefix` with
`/opt/homebrew` or `/usr/local`.

You will build:

- `lassie.app`, the graphical editor
- `cmod`, the composition and synthesis engine

## What do I need to install?

Do not download each dependency from a separate website. The table below
describes the recommended Homebrew path: follow steps 1-3 and the installers
will put everything in the correct location. If you already use MacPorts, skip
Homebrew and follow the separate MacPorts section instead.

| Requirement | How it is provided |
| --- | --- |
| Git | Installed with Apple's Command Line Tools in step 1 |
| C++20 compiler | Apple Clang, installed with Command Line Tools in step 1 |
| Homebrew | Installed in step 2, then used in step 3 to install the remaining dependencies |
| CMake 3.25 or newer | Installed by Homebrew in step 3 |
| libsndfile | Installed by Homebrew in step 3 |
| Qt 6.8 or newer | Installed as `qt@6` by Homebrew in step 3 |
| muParser | Already included in this repository; nothing to install |
| pugixml | Already included in this repository; nothing to install |

In short, you install Apple's Command Line Tools and Homebrew, then run one
Homebrew command. CMake builds the included copies of muParser and pugixml
automatically.

## Quick start with Homebrew

Homebrew is the recommended route for first-time builders. Run every command
below in Terminal. Do not use MacPorts packages in the same build.

### 1. Install Apple's command-line tools

Run:

```sh
xcode-select --install
```

When the macOS dialog appears, click **Install**, accept the license, and wait
for it to finish. If macOS says the tools are already installed, continue to
the checks below. Command Line Tools provides both Git and the Apple Clang C++
compiler, so you do not need to install GCC or the full Xcode application.

```sh
xcode-select -p
git --version
/usr/bin/clang++ --version
printf '#include <algorithm>\nint main() { return 0; }\n' | /usr/bin/clang++ -std=c++20 -x c++ -fsyntax-only -
```

The first command prints the active developer-tools directory. The next two
print a Git version and an `Apple clang` version. The last command prints
nothing when the C++20 toolchain is working. If any command fails, or the last
command reports `algorithm file not found`, stop and use the matching fix in
[Troubleshooting](#troubleshooting).

### 2. Install Homebrew

If `brew --version` already works, skip to step 3. Otherwise, copy the current
installer command from [brew.sh](https://brew.sh/) or run:

```sh
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

The installer first shows what it will change. Press **Return** when it asks to
continue. It may then request an administrator password. Terminal does not
display characters while you type a password; this is normal. At the end, copy
and run the `Next steps` commands printed by Homebrew, then open a new Terminal
window. Those commands make `brew` available in future Terminal windows.

Confirm that Homebrew is available:

```sh
brew --version
```

Do not run `sudo brew install ...`. Homebrew only needs administrator approval
during its initial installation.

### 3. Install the build dependencies

```sh
brew install cmake libsndfile qt@6
```

This single command installs CMake, libsndfile, Qt 6, and the supporting
libraries they need. Homebrew downloads the correct builds for Apple silicon
or Intel automatically. It is safe to run the command again: Homebrew keeps
packages that are already current.

Verify the result:

```sh
cmake --version
brew list --versions cmake libsndfile qt@6
```

The output must list all three packages. Installed versions may be newer than
the stated minimums; CMake must be 3.25 or newer and Qt must be 6.8 or newer.
Do not install muParser or pugixml separately.

### 4. Download the source

```sh
git clone https://github.com/cmp-illinois/DISSCO.git
cd DISSCO
```

If you already cloned the repository, open Terminal in that existing `DISSCO`
directory instead.

If the prompt begins with `(base)`, deactivate Conda before configuring:

```sh
conda deactivate
```

### 5. Configure a clean Release build

Run this command from the repository root, the directory containing the top-level
`CMakeLists.txt` file:

```sh
cmake --fresh -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6);$(brew --prefix libsndfile)"
```

`--fresh` clears stale CMake toolchain settings without deleting your source
files. A successful configuration ends with `Configuring done`, `Generating
done`, and `Build files have been written to`.

You may also see `Could NOT find WrapVulkanHeaders`. DISSCO does not require
Vulkan, so this line is harmless when configuration finishes successfully.

### 6. Build, test, and run DISSCO

```sh
cmake --build build --parallel
ctest --test-dir build --output-on-failure
open build/LASSIE/lassie.app
```

The build is successful when it reaches `100%`, the tests report `100% tests
passed`, and LASSIE opens. The command-line engine is at `build/CMOD/cmod`.
Most users can stop here.

## MacPorts alternative (existing MacPorts users only)

Use this route only if you already manage development packages with MacPorts.
Do not install some DISSCO dependencies with Homebrew and others with MacPorts.
Complete the command-line-tools checks in step 1 before installing any ports.
If `port version` does not work, first follow the official
[MacPorts installation guide](https://guide.macports.org/chunked/installing.html).

Install the dependencies:

```sh
sudo port selfupdate
sudo port install cmake libsndfile qt6
```

This installs the same three external dependencies as the Homebrew command.
Command Line Tools still provides Git and Apple Clang, while muParser and
pugixml still come from the DISSCO repository.

Then use this configuration command instead of step 5:

```sh
cmake --fresh -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
  -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt6;/opt/local"
```

Continue with the build, test, and run commands in step 6. If MacPorts warns
that old C++ headers are present, do not continue until you apply the documented
CLT 16+ fix below.

## Troubleshooting

### `algorithm file not found` or MacPorts reports old C++ headers

First make sure the current Apple tools are installed:

```sh
xcode-select --install
```

Command Line Tools 16 and newer can leave an obsolete header directory behind
when upgrading an older installation. Check for that exact directory:

```sh
ls -ld /Library/Developer/CommandLineTools/usr/include/c++
```

If that directory exists and either MacPorts reported the old-headers warning
or the compiler check in step 1 failed, remove the obsolete directory using the
[MacPorts CLT 16+ fix](https://trac.macports.org/wiki/ProblemHotlist#clts16):

```sh
sudo rm -rf /Library/Developer/CommandLineTools/usr/include/c++
```

Copy that path exactly and do not add wildcards. Re-run the compiler check from
step 1. MacPorts users should also clean the port that failed before retrying;
for the previously observed `gperf` failure:

```sh
sudo port clean gperf
sudo port install libsndfile qt6
```

### CMake uses a Conda or third-party compiler

Both configuration commands explicitly select Apple Clang. If an old shell
environment still injects compiler flags, run:

```sh
conda deactivate 2>/dev/null || true
unset CC CXX CFLAGS CXXFLAGS CPPFLAGS LDFLAGS CPATH CPLUS_INCLUDE_PATH SDKROOT DEVELOPER_DIR MACOSX_DEPLOYMENT_TARGET
```

Then repeat the `cmake --fresh` configuration command for your package manager:
step 5 for Homebrew, or the command in the MacPorts alternative. Do not edit or
remove the project's `#include <algorithm>` line; it is a standard C++ header
supplied by Apple's toolchain.

### CMake cannot find Qt or libsndfile

Use only the configuration command for the package manager that installed your
dependencies. Check what is installed with one of:

```sh
brew list --versions libsndfile qt@6
```

```sh
port installed libsndfile qt6
```

Then repeat the matching `cmake --fresh` command. Installing Vulkan or
`pkg-config` does not fix a missing Qt or C++ standard-library installation.

### Homebrew reports `Need sudo access on macOS`

Run the official Homebrew installer interactively from an administrator account,
so macOS can request authorization. Do not add `sudo` to `brew install` and do
not paste an administrator password into a command or chat message.

### Which warnings can be ignored?

These CMake feature checks are not build failures when configuration continues
to `Configuring done`:

- `Could NOT find WrapVulkanHeaders`
- `HAVE_STDATOMIC_WITH_LIB - Failed` followed by `Found WrapAtomic: TRUE`

The first `fatal error:` or final nonzero build result is the useful failure to
report.

## Optional: build installable packages

Normal development builds do not need `dylibbundler`. Install it only when you
want a DMG or standalone CMOD archive:

```sh
brew install dylibbundler
```

MacPorts users should run this instead:

```sh
sudo port install dylibbundler
```

Build the DMG:

```sh
cmake --build build --target package
```

The result is `build/DISSCO-<version>-Darwin.dmg`. It contains `lassie.app`,
with `cmod` embedded at `Contents/MacOS/cmod` and its non-system libraries
bundled inside the application.

Build the standalone CMOD archive with:

```sh
cmake --build build --target cmod-package
```

The result is `build/CMOD-<version>-Darwin-<architecture>.tar.gz`.

Local DMGs use an ad-hoc signature and are intended for testing on the machine
that built them. Official release DMGs must use a Developer ID Application
certificate, hardened runtime, notarization, and a stapled notarization ticket.

<details>
<summary>Release maintainer configuration (not needed for local builds)</summary>

The GitHub Actions release workflow requires these secrets:

- `MACOS_CERTIFICATE_P12`: base64-encoded Developer ID Application `.p12`
  certificate
- `MACOS_CERTIFICATE_PASSWORD`: password used when exporting the `.p12`
- `APPLE_ID`: Apple Developer account email used by `notarytool`
- `APPLE_TEAM_ID`: Apple Developer team identifier
- `APPLE_APP_SPECIFIC_PASSWORD`: app-specific password for the Apple ID

Without these secrets, a release build still uploads a DMG, but it is only
ad-hoc signed and not notarized: the workflow prints an "Unsigned macOS
release" warning, and macOS asks users to approve the app on first launch.
Workflow runs without a release tag also produce ad-hoc signed, temporary
artifacts for developer testing.

The application icon at `packaging/macos/LASSIE.icns` is a placeholder. Replace
the artwork and run `packaging/macos/make-icns.sh` before an official release.

</details>
