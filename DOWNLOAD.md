# Download DISSCO

Go to the [latest DISSCO release](https://github.com/cmp-illinois/DISSCO/releases/latest) page and scroll down to the assets section for the downloads. Below are detailed the specific install instructions for each operating system.

DISSCO includes three components:

- **CMOD**: the composition module.
- **LASS**: the sound synthesis library used by CMOD. It is built into the application and needs no separate download or installation.
- **LASSIE**: the graphical editor for creating and running DISSCO projects.

Download a `DISSCO-*` asset for the complete application. The optional `CMOD-*` assets provide CMOD with LASS for command-line use, without LASSIE.

AppImage is a Linux format. The standalone CMOD download for Windows is a `.zip` archive.

Released binaries are currently available for Windows x64 and Linux x86_64. macOS downloads are [temporarily unavailable](#macos). If there is no asset for your CPU architecture, use the [source-build guides](#building-from-source) instead of running a mismatched binary.

## Windows 10 or 11 (x64)

### Complete DISSCO application

1. Download `DISSCO-<version>-Windows.exe`.
2. Run the installer and follow its prompts. Windows may request administrator approval because the installer registers `.dissco` files for all users.
3. Start LASSIE from the Start menu or by opening a `.dissco` file.

The installer includes all three DISSCO components, Qt, and CMOD's audio libraries.

### Standalone CMOD

1. Download `CMOD-<version>-Windows-x64.zip`.
2. Extract the entire ZIP. Do not run `cmod.exe` from inside the ZIP or copy it away from the included DLL files.
3. Open a terminal in the extracted folder and run:

   ```powershell
   .\Run-CMOD.bat --help
   .\Run-CMOD.bat C:\path\to\project.dissco
   ```

The archive includes the Microsoft Visual C++ runtime and CMOD's audio libraries. It does not require administrator access.

## macOS

> **macOS downloads are temporarily unavailable.** No macOS release has been published yet. Developers can still [build from source](BUILDING_MACOS.md).

## Linux

Choose the asset whose architecture matches `uname -m`. The current automated release uses `x86_64`.

### Complete DISSCO application

Download `DISSCO-<version>-Linux-<architecture>.AppImage`, then make it executable and run it:

```bash
chmod +x DISSCO-<version>-Linux-<architecture>.AppImage
./DISSCO-<version>-Linux-<architecture>.AppImage
```

### Standalone CMOD

Download `CMOD-<version>-Linux-<architecture>.AppImage`, then make it executable and run it:

```bash
chmod +x CMOD-<version>-Linux-<architecture>.AppImage
./CMOD-<version>-Linux-<architecture>.AppImage --help
./CMOD-<version>-Linux-<architecture>.AppImage /path/to/project.dissco
```

See the [AppImage quickstart](https://docs.appimage.org/introduction/quickstart.html) for graphical instructions and troubleshooting.

## Score and PDF output

All release packages include the libraries needed for audio synthesis. They do not include LilyPond. Install [LilyPond](https://lilypond.org/download.html) separately and make its executable available on `PATH` when you need score or PDF output from either DISSCO or standalone CMOD.

## Verify the download

Download `SHA256SUMS` from the same release. Compare the checksum for the asset you downloaded before running it.

On Linux:

```bash
sha256sum DISSCO-<version>-Linux-<architecture>.AppImage
```

On Windows PowerShell:

```powershell
Get-FileHash .\DISSCO-<version>-Windows.exe -Algorithm SHA256
```

The printed hash must match the corresponding line in `SHA256SUMS`.

## Building from source

These are download instructions for released binaries. Developers who need to build from source should use [BUILDING_LINUX.md](BUILDING_LINUX.md), [BUILDING_MACOS.md](BUILDING_MACOS.md), or [BUILDING_WINDOWS.md](BUILDING_WINDOWS.md).
