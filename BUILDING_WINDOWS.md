Building on Windows 
=================
(WIP, not yet building without some hacking)

What is marked as not yet supported (denoted by ❌ and understood as referring to the entire line/paragraph) will be supported, soon.

We intend to offer static binaries at some point.

Preliminary Requirements
--------------------------

The following are *necessary* to compile CMOD and LASS:

- git
- Visual Studio 2022,
- A C/C++ compiler (like MSVC `cl`),
- cmake >= 3.25,
- vcpkg >= 2024.X.X
- libsndfile >= 1.0,
- libxerces-c >= 3.2, and
<!-- - muparser >= 2.X -->

To compile with LASSIE, the following is *necessary*:

- (🟡) Qt >= 6.8

Note: There are definitely other ways to compile DISSCO on Windows, but this is the way the author decided to go.

Installing requirements and recommendations:
--------------------------------------------

<!-- *For all methods*, it's worth keeping in mind that we statically link `muparser`, meaning you don't need to worry about installing it. Please report any issues related to muparser during compilation. -->

You should [install Visual Studio 2022](https://visualstudio.microsoft.com/) from Microsoft and, in the process, install the Microsoft Visual C/C++ compilers, CMake, and vcpkg. You will need to run all the commands given hereafter in **Developer PowerShell**, which you can access either within a VS project or through the PowerShell script provided at `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1`.

If you didn't install CMake during the VS install or don't want to install through VS, you can get an installer [from CMake directly](https://cmake.org/download/).

Dependency management will be handled via `vcpkg`. **It's worth reminding that this should be done in Developer PowerShell.** If you didn't install during the VS install, you can build `vcpkg` from source, like so:

    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg; .\bootstrap-vcpkg.bat

You should then set two environmental variables to simplify our scripts:

    $env:VCPKG_ROOT = "C:\path\to\vcpkg"
    $env:PATH = "$env:VCPKG_ROOT;$env:PATH"

We'll come back to `vcpkg` in the Building section.

Installing DISSCO
-----------------
Just `git clone` this repo; explicitly:

    git clone https://github.com/cmp-illinois/DISSCO-2.2.0.git

Building
--------

**Dependencies with `vcpkg`**

From the project's root directory (by default: `C:\path\to\DISSCO-X.X.X`), run the following commands to tell `vcpkg` what dependencies you'll need when building:

    vcpkg new --application

which will create a `vcpkg-configuration.json` file, and

    vcpkg add port libsndfile
    vcpkg add port xerces-c

which will create a `vcpkg.json` file and add dependencies to it.

Now, make a `CMakeUserPresets.json` file with the following:
```json
{
    "version": 2,
    "configurePresets": [
        {
            "name": "windows",
            "inherits": "vcpkg",
            "environment": {
                "VCPKG_ROOT": "C:/path/to/vcpkg"
            }
        }
    ]
}
```
where `C:/path/to/vcpkg` is the path to the directory you installed `vcpkg` in.

Finally, run `cmake --preset=windows -G Ninja`.

**(❌) Rest of the Build** 

From `build/`, do

    cmake --build .

to build.

By running this command in `build`, one generates a so-called *out-of-source* (OOS) build. The alternative, an in-source build, is heavily discouraged (including [by the CMake maintainers](https://cmake.org/cmake/help/book/mastering-cmake/chapter/Getting%20Started.html#directory-structure)), and the root `CMakeLists.txt` reflects this distaste. The rationale is that OOS builds minimize clutter and collect all build files in one directory, whereas in-source builds put build files virtually everywhere. (This is bad.)

From `build`, you can clean `build` using `cmake --build . --target clean`. Alternatively, you can do `rmdir /s build` from outside of `build`.

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