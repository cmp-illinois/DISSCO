[CmdletBinding()]
param(
    [string]$ProjectRoot,
    [string]$QtBin,
    [string]$LilyPondRoot,
    [string]$BuildDirectory = "build",
    [string]$PackageName = "DISSCO-Windows-x64",
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Release",
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$SkipSmokeTest,
    [switch]$NoZip
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Resolve-RequiredDirectory {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        throw "$Label directory was not found: $Path"
    }

    return (Resolve-Path -LiteralPath $Path).Path
}

function Get-CMakeCacheValue {
    param(
        [Parameter(Mandatory = $true)][string]$CachePath,
        [Parameter(Mandatory = $true)][string]$Name
    )

    if (-not (Test-Path -LiteralPath $CachePath -PathType Leaf)) {
        return $null
    }

    $match = Select-String -LiteralPath $CachePath -Pattern ("^" + [regex]::Escape($Name) + ":[^=]+=(.*)$") |
        Select-Object -First 1
    if ($null -eq $match) {
        return $null
    }

    return $match.Matches[0].Groups[1].Value
}

function Find-BuiltExecutable {
    param(
        [Parameter(Mandatory = $true)][string]$BuildRoot,
        [Parameter(Mandatory = $true)][string]$Component,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Configuration
    )

    $preferred = @(
        (Join-Path $BuildRoot "$Component\$Configuration\$Name"),
        (Join-Path $BuildRoot "$Component\$Name")
    )
    foreach ($candidate in $preferred) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return Get-Item -LiteralPath $candidate
        }
    }

    $matches = @(
        Get-ChildItem -LiteralPath $BuildRoot -Recurse -File -Filter $Name |
            Where-Object {
                $_.FullName -notmatch '[\\/]CMakeFiles[\\/]' -and
                $_.FullName -notmatch '[\\/]publish-bin[\\/]'
            } |
            Sort-Object LastWriteTime -Descending
    )
    if ($matches.Count -eq 0) {
        throw "$Name was not found under: $BuildRoot"
    }

    Write-Warning "Using non-canonical build output: $($matches[0].FullName)"
    return $matches[0]
}

function Find-QtBin {
    param(
        [string]$RequestedQtBin,
        [Parameter(Mandatory = $true)][string]$CachePath
    )

    $candidates = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($RequestedQtBin)) {
        $candidates.Add($RequestedQtBin)
    }

    $qt6Dir = Get-CMakeCacheValue -CachePath $CachePath -Name "Qt6_DIR"
    if (-not [string]::IsNullOrWhiteSpace($qt6Dir)) {
        $qtRoot = [System.IO.Path]::GetFullPath((Join-Path $qt6Dir "..\..\.."))
        $candidates.Add((Join-Path $qtRoot "bin"))
    }

    $command = Get-Command "windeployqt.exe" -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        $candidates.Add((Split-Path -Parent $command.Source))
    }

    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        if (Test-Path -LiteralPath (Join-Path $candidate "windeployqt.exe") -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Could not find Qt's windeployqt.exe. Pass -QtBin <QtRoot\bin>."
}

function Get-LilyPondInfo {
    param([Parameter(Mandatory = $true)][string]$Candidate)

    if (-not (Test-Path -LiteralPath $Candidate)) {
        return $null
    }

    $resolved = (Resolve-Path -LiteralPath $Candidate).Path
    if (Test-Path -LiteralPath $resolved -PathType Leaf) {
        if ([System.IO.Path]::GetFileName($resolved) -ine "lilypond.exe") {
            return $null
        }
        $exe = Get-Item -LiteralPath $resolved
        $root = $exe.Directory
    }
    else {
        $root = Get-Item -LiteralPath $resolved
        $exe = Get-ChildItem -LiteralPath $root.FullName -Recurse -File -Filter "lilypond.exe" |
            Select-Object -First 1
        if ($null -eq $exe) {
            return $null
        }
    }

    if ($exe.Directory.Name -ieq "bin") {
        $root = $exe.Directory.Parent
    }

    $rootPath = $root.FullName.TrimEnd('\', '/')
    if (-not $exe.DirectoryName.StartsWith(
        $rootPath,
        [System.StringComparison]::OrdinalIgnoreCase)) {
        return $null
    }

    $relativeBin = $exe.DirectoryName.Substring($rootPath.Length).TrimStart('\', '/')
    return [pscustomobject]@{
        Root = $rootPath
        Exe = $exe.FullName
        RelativeBin = $relativeBin
    }
}

function Find-LilyPond {
    param(
        [string]$RequestedRoot,
        [Parameter(Mandatory = $true)][string]$ProjectRoot,
        [Parameter(Mandatory = $true)][string]$PackageName
    )

    $candidates = [System.Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($RequestedRoot)) {
        $candidates.Add($RequestedRoot)
    }

    $command = Get-Command "lilypond.exe" -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        $candidates.Add($command.Source)
    }

    # This fallback lets an existing portable package supply LilyPond when
    # rebuilding on a machine where LilyPond is no longer installed.
    $candidates.Add((Join-Path $ProjectRoot "dist\$PackageName\tools\lilypond"))

    foreach ($base in @($env:ProgramFiles, ${env:ProgramFiles(x86)})) {
        if ([string]::IsNullOrWhiteSpace($base) -or
            -not (Test-Path -LiteralPath $base -PathType Container)) {
            continue
        }
        Get-ChildItem -LiteralPath $base -Directory -Filter "LilyPond*" -ErrorAction SilentlyContinue |
            ForEach-Object { $candidates.Add($_.FullName) }
    }

    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        $info = Get-LilyPondInfo -Candidate $candidate
        if ($null -ne $info) {
            return $info
        }
    }

    throw "Could not find LilyPond. Pass -LilyPondRoot <directory containing LilyPond>."
}

function Find-MsvcRuntimeDirectory {
    param([Parameter(Mandatory = $true)][string]$CachePath)

    $candidates = [System.Collections.Generic.List[string]]::new()
    $redistFromEnvironment = [Environment]::GetEnvironmentVariable("VCToolsRedistDir", "Process")
    if (-not [string]::IsNullOrWhiteSpace($redistFromEnvironment)) {
        Get-ChildItem -LiteralPath (Join-Path $redistFromEnvironment "x64") -Directory -Filter "Microsoft.VC*.CRT" -ErrorAction SilentlyContinue |
            ForEach-Object { $candidates.Add($_.FullName) }
    }

    $compiler = Get-CMakeCacheValue -CachePath $CachePath -Name "CMAKE_CXX_COMPILER"
    if (-not [string]::IsNullOrWhiteSpace($compiler)) {
        $match = [regex]::Match(
            $compiler,
            '^(.*[\\/]VC)[\\/]Tools[\\/]MSVC[\\/]([^\\/]+)[\\/]')
        if ($match.Success) {
            $vcRoot = $match.Groups[1].Value
            $compilerVersion = $match.Groups[2].Value
            $redistRoot = Join-Path $vcRoot "Redist\MSVC"

            foreach ($versionRoot in @(
                (Join-Path $redistRoot $compilerVersion),
                $redistRoot
            )) {
                if (-not (Test-Path -LiteralPath $versionRoot -PathType Container)) {
                    continue
                }
                Get-ChildItem -LiteralPath $versionRoot -Recurse -Directory -Filter "Microsoft.VC*.CRT" -ErrorAction SilentlyContinue |
                    Where-Object {
                        $_.FullName -match '[\\/]x64[\\/]' -and
                        $_.FullName -notmatch '[\\/]onecore[\\/]'
                    } |
                    Sort-Object FullName -Descending |
                    ForEach-Object { $candidates.Add($_.FullName) }
            }
        }
    }

    foreach ($candidate in $candidates | Select-Object -Unique) {
        $required = @("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll")
        $missing = @($required | Where-Object {
            -not (Test-Path -LiteralPath (Join-Path $candidate $_) -PathType Leaf)
        })
        if ($missing.Count -eq 0) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Could not find the x64 MSVC runtime DLLs. Install the Visual Studio C++ build tools or set VCToolsRedistDir."
}

function Import-VisualStudioEnvironment {
    param([Parameter(Mandatory = $true)][string]$CachePath)

    $compiler = Get-CMakeCacheValue -CachePath $CachePath -Name "CMAKE_CXX_COMPILER"
    if ([string]::IsNullOrWhiteSpace($compiler)) {
        throw "CMAKE_CXX_COMPILER is missing from CMakeCache.txt."
    }

    $match = [regex]::Match(
        $compiler,
        '^(.*[\\/]VC)[\\/]Tools[\\/]MSVC[\\/][^\\/]+[\\/]')
    if (-not $match.Success) {
        # Non-MSVC toolchains do not need Visual Studio's vcvars environment.
        return
    }

    $vcVars = Join-Path $match.Groups[1].Value "Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path -LiteralPath $vcVars -PathType Leaf)) {
        throw "Visual Studio's vcvars64.bat was not found: $vcVars"
    }

    $commandLine = "call `"$vcVars`" >nul && set"
    $environmentLines = & $env:ComSpec /d /s /c $commandLine
    if ($LASTEXITCODE -ne 0) {
        throw "vcvars64.bat failed with exit code $LASTEXITCODE."
    }

    foreach ($line in $environmentLines) {
        $separator = $line.IndexOf("=")
        if ($separator -le 0) {
            continue
        }
        $name = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        [Environment]::SetEnvironmentVariable($name, $value, "Process")
    }
}

function Copy-ExecutableAndAdjacentDlls {
    param(
        [Parameter(Mandatory = $true)][System.IO.FileInfo]$Executable,
        [Parameter(Mandatory = $true)][string]$Destination
    )

    Copy-Item -LiteralPath $Executable.FullName -Destination $Destination -Force
    Get-ChildItem -LiteralPath $Executable.DirectoryName -File -Filter "*.dll" -ErrorAction SilentlyContinue |
        ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $Destination -Force
        }
}

function Assert-DirectChildPath {
    param(
        [Parameter(Mandatory = $true)][string]$Parent,
        [Parameter(Mandatory = $true)][string]$Child
    )

    $resolvedParent = [System.IO.Path]::GetFullPath($Parent).TrimEnd('\', '/')
    $resolvedChildParent = [System.IO.Path]::GetDirectoryName(
        [System.IO.Path]::GetFullPath($Child)).TrimEnd('\', '/')
    if (-not $resolvedParent.Equals(
        $resolvedChildParent,
        [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to modify a path outside the expected directory: $Child"
    }
}

function Get-PeMachine {
    param([Parameter(Mandatory = $true)][string]$Path)

    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $reader = [System.IO.BinaryReader]::new($stream)
        if ($reader.ReadUInt16() -ne 0x5A4D) {
            throw "Not a PE file: $Path"
        }
        $stream.Position = 0x3C
        $peOffset = $reader.ReadInt32()
        $stream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x00004550) {
            throw "Invalid PE signature: $Path"
        }
        return $reader.ReadUInt16()
    }
    finally {
        $stream.Dispose()
    }
}

function Invoke-PortableSmokeTests {
    param(
        [Parameter(Mandatory = $true)][string]$PackageRoot,
        [Parameter(Mandatory = $true)][string]$BundledLilyPondExe
    )

    $savedEnvironment = @{}
    foreach ($name in @("PATH", "QT_PLUGIN_PATH", "QML2_IMPORT_PATH")) {
        $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, "Process")
    }

    $guiProcess = $null
    try {
        # Exclude the developer's Qt, LilyPond, and Visual Studio paths.
        $windowsRoot = [Environment]::GetEnvironmentVariable("SystemRoot", "Process")
        [Environment]::SetEnvironmentVariable(
            "PATH",
            "$windowsRoot\System32;$windowsRoot",
            "Process")
        [Environment]::SetEnvironmentVariable("QT_PLUGIN_PATH", $null, "Process")
        [Environment]::SetEnvironmentVariable("QML2_IMPORT_PATH", $null, "Process")

        $cmodOutput = & (Join-Path $PackageRoot "CMOD.exe") --help 2>&1
        if ($LASTEXITCODE -ne 0 -or ($cmodOutput -join "`n") -notmatch "Usage:") {
            throw "CMOD smoke test failed with exit code $LASTEXITCODE."
        }

        $lilyOutput = & $BundledLilyPondExe --version 2>&1
        if ($LASTEXITCODE -ne 0 -or ($lilyOutput -join "`n") -notmatch "GNU LilyPond") {
            throw "Bundled LilyPond smoke test failed with exit code $LASTEXITCODE."
        }

        $guiProcess = Start-Process `
            -FilePath (Join-Path $PackageRoot "LASSIE.exe") `
            -WorkingDirectory $PackageRoot `
            -WindowStyle Hidden `
            -PassThru
        Start-Sleep -Seconds 3
        if ($guiProcess.HasExited) {
            throw "LASSIE exited during the portable smoke test (exit code $($guiProcess.ExitCode))."
        }
    }
    finally {
        if ($null -ne $guiProcess -and -not $guiProcess.HasExited) {
            Stop-Process -Id $guiProcess.Id -Force
            $guiProcess.WaitForExit()
        }
        foreach ($name in $savedEnvironment.Keys) {
            [Environment]::SetEnvironmentVariable(
                $name,
                $savedEnvironment[$name],
                "Process")
        }
    }
}

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    $ProjectRoot = $PSScriptRoot
}
$ProjectRoot = Resolve-RequiredDirectory -Path $ProjectRoot -Label "ProjectRoot"

if (-not [System.IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path $ProjectRoot $BuildDirectory
}
$BuildRoot = Resolve-RequiredDirectory -Path $BuildDirectory -Label "Build"
$CachePath = Join-Path $BuildRoot "CMakeCache.txt"
if (-not (Test-Path -LiteralPath $CachePath -PathType Leaf)) {
    throw "CMakeCache.txt was not found in the build directory: $BuildRoot"
}

Import-VisualStudioEnvironment -CachePath $CachePath

$CMakeExe = Get-CMakeCacheValue -CachePath $CachePath -Name "CMAKE_COMMAND"
if ([string]::IsNullOrWhiteSpace($CMakeExe) -or
    -not (Test-Path -LiteralPath $CMakeExe -PathType Leaf)) {
    $cmakeCommand = Get-Command "cmake.exe" -ErrorAction SilentlyContinue
    if ($null -eq $cmakeCommand) {
        throw "Could not find cmake.exe from CMakeCache.txt or PATH."
    }
    $CMakeExe = $cmakeCommand.Source
}
$CTestExe = Join-Path (Split-Path -Parent $CMakeExe) "ctest.exe"
if (-not (Test-Path -LiteralPath $CTestExe -PathType Leaf)) {
    $ctestCommand = Get-Command "ctest.exe" -ErrorAction SilentlyContinue
    if ($null -eq $ctestCommand) {
        throw "Could not find ctest.exe next to CMake or in PATH."
    }
    $CTestExe = $ctestCommand.Source
}

$QtBin = Find-QtBin -RequestedQtBin $QtBin -CachePath $CachePath
$LilyPond = Find-LilyPond `
    -RequestedRoot $LilyPondRoot `
    -ProjectRoot $ProjectRoot `
    -PackageName $PackageName
$MsvcRuntimeDirectory = Find-MsvcRuntimeDirectory -CachePath $CachePath

$DistRoot = Join-Path $ProjectRoot "dist"
New-Item -ItemType Directory -Path $DistRoot -Force | Out-Null
$DistRoot = (Resolve-Path -LiteralPath $DistRoot).Path
$PackageRoot = Join-Path $DistRoot $PackageName
$ZipPath = Join-Path $DistRoot ($PackageName + ".zip")
$ChecksumPath = $ZipPath + ".sha256"
$StageRoot = Join-Path $DistRoot (".$PackageName.staging-$PID")

Assert-DirectChildPath -Parent $DistRoot -Child $PackageRoot
Assert-DirectChildPath -Parent $DistRoot -Child $StageRoot
if (Test-Path -LiteralPath $StageRoot) {
    Remove-Item -LiteralPath $StageRoot -Recurse -Force
}

try {
    if (-not $SkipBuild) {
        Write-Host "[1/10] Building current $Configuration binaries..." -ForegroundColor Cyan
        & $CMakeExe --build $BuildRoot --config $Configuration --parallel
        if ($LASTEXITCODE -ne 0) {
            throw "$Configuration build failed."
        }
    }
    else {
        Write-Host "[1/10] Build skipped by request." -ForegroundColor DarkGray
    }

    if (-not $SkipTests) {
        Write-Host "[2/10] Running tests..." -ForegroundColor Cyan
        & $CTestExe --test-dir $BuildRoot -C $Configuration --output-on-failure
        if ($LASTEXITCODE -ne 0) {
            throw "Tests failed."
        }
    }
    else {
        Write-Host "[2/10] Tests skipped by request." -ForegroundColor DarkGray
    }

    Write-Host "[3/10] Selecting fresh build outputs..." -ForegroundColor Cyan
    $LassieExe = Find-BuiltExecutable `
        -BuildRoot $BuildRoot `
        -Component "LASSIE" `
        -Name "LASSIE.exe" `
        -Configuration $Configuration
    $CmodExe = Find-BuiltExecutable `
        -BuildRoot $BuildRoot `
        -Component "CMOD" `
        -Name "CMOD.exe" `
        -Configuration $Configuration
    Write-Host "LASSIE: $($LassieExe.FullName)"
    Write-Host "CMOD:   $($CmodExe.FullName)"
    Write-Host "Qt:     $QtBin"
    Write-Host "LilyPond: $($LilyPond.Root)"
    Write-Host "MSVC runtime: $MsvcRuntimeDirectory"

    Write-Host "[4/10] Creating an isolated staging directory..." -ForegroundColor Cyan
    New-Item -ItemType Directory -Path $StageRoot -Force | Out-Null
    Copy-ExecutableAndAdjacentDlls -Executable $LassieExe -Destination $StageRoot
    Copy-ExecutableAndAdjacentDlls -Executable $CmodExe -Destination $StageRoot

    Write-Host "[5/10] Deploying Qt and app-local MSVC runtime DLLs..." -ForegroundColor Cyan
    $WinDeployQt = Join-Path $QtBin "windeployqt.exe"
    & $WinDeployQt `
        --release `
        --force `
        --no-compiler-runtime `
        --no-translations `
        --no-quick-import `
        --dir $StageRoot `
        (Join-Path $StageRoot "LASSIE.exe")
    if ($LASTEXITCODE -ne 0) {
        throw "windeployqt failed."
    }

    Get-ChildItem -LiteralPath $MsvcRuntimeDirectory -File -Filter "*.dll" |
        ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $StageRoot -Force
        }
    Get-ChildItem -LiteralPath $StageRoot -File -Filter "vc_redist*.exe" -ErrorAction SilentlyContinue |
        Remove-Item -Force

    Write-Host "[6/10] Bundling LilyPond and documentation..." -ForegroundColor Cyan
    $BundledLilyPondRoot = Join-Path $StageRoot "tools\lilypond"
    New-Item -ItemType Directory -Path $BundledLilyPondRoot -Force | Out-Null
    Get-ChildItem -LiteralPath $LilyPond.Root -Force |
        ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $BundledLilyPondRoot -Recurse -Force
        }

    $BundledLilyBin = if ([string]::IsNullOrWhiteSpace($LilyPond.RelativeBin)) {
        $BundledLilyPondRoot
    }
    else {
        Join-Path $BundledLilyPondRoot $LilyPond.RelativeBin
    }
    $BundledLilyPondExe = Join-Path $BundledLilyBin "lilypond.exe"

    if (Test-Path -LiteralPath (Join-Path $ProjectRoot "Documents") -PathType Container) {
        Copy-Item `
            -LiteralPath (Join-Path $ProjectRoot "Documents") `
            -Destination (Join-Path $StageRoot "Documentation") `
            -Recurse `
            -Force
    }
    Copy-Item -LiteralPath (Join-Path $ProjectRoot "LICENSE") -Destination $StageRoot -Force
    New-Item -ItemType Directory -Path (Join-Path $StageRoot "SoundFiles") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $StageRoot "ScoreFiles") -Force | Out-Null

    Write-Host "[7/10] Writing portable launchers and package information..." -ForegroundColor Cyan
    $launcherLilyBin = $BundledLilyBin.Substring($StageRoot.Length).TrimStart('\', '/')
    $launcher = @"
@echo off
setlocal
cd /d "%~dp0"
set "PATH=%~dp0$launcherLilyBin;%PATH%"
start "" "%~dp0LASSIE.exe" %*
endlocal
"@
    Set-Content `
        -LiteralPath (Join-Path $StageRoot "Run-DISSCO.bat") `
        -Value $launcher `
        -Encoding ASCII

    $versionFile = Join-Path $ProjectRoot "DISSCOVersions.txt"
    $versionText = Get-Content -LiteralPath $versionFile -Raw
    $major = [regex]::Match($versionText, 'DISSCO_VER_MAJOR\s+(\d+)').Groups[1].Value
    $minor = [regex]::Match($versionText, 'DISSCO_VER_MINOR\s+(\d+)').Groups[1].Value
    $patch = [regex]::Match($versionText, 'DISSCO_VER_PATCH\s+(\d+)').Groups[1].Value
    $version = "$major.$minor.$patch"
    $commit = (& git -C $ProjectRoot rev-parse --short HEAD 2>$null)
    if ($LASTEXITCODE -ne 0) {
        $commit = "unknown"
    }
    $dirtySuffix = if ((& git -C $ProjectRoot status --porcelain 2>$null)) {
        " + local changes"
    }
    else {
        ""
    }

    # Keep this script ASCII-compatible for Windows PowerShell 5, which reads
    # UTF-8-without-BOM source files using the active Windows code page.
    $chineseInstructions = [Text.Encoding]::UTF8.GetString(
        [Convert]::FromBase64String(
            "5Lit5paH6K+05piOCiAg6Kej5Y6L5pW05LiqIFpJUCDlkI7vvIznm7TmjqXlj4zlh7sgTEFTU0lFLmV4ZeOAggogIOS4jemcgOimgeWuieijhSBRdOOAgVZpc3VhbCBDKysg6L+Q6KGM5bqT5oiWIExpbHlQb25k77yM5Lmf5LiN6ZyA6KaB566h55CG5ZGY5p2D6ZmQ44CCCiAg6K+35Yu/5Y+q5aSN5Yi25Y2V5LiqIEVYRe+8m3Rvb2xz44CBcGxhdGZvcm1zIOWSjCBETEwg5paH5Lu26YO95piv56iL5bqP55qE5LiA6YOo5YiG44CC"))

    $readme = @"
DISSCO $version Portable for Windows x64
========================================

RUN
  Double-click LASSIE.exe.
  Run-DISSCO.bat is also provided as a compatibility launcher.

REQUIREMENTS
  - 64-bit Windows 10 or Windows 11
  - Extract the entire ZIP before running it
  - Keep every file and folder together

Qt, the Microsoft Visual C++ runtime, CMOD, and LilyPond are included.
No installer, administrator access, or separately installed dependency is
required. Project output is written next to the project file, not into the
application installation.

$chineseInstructions

Build: $commit$dirtySuffix
"@
    Set-Content `
        -LiteralPath (Join-Path $StageRoot "README.txt") `
        -Value $readme `
        -Encoding UTF8

    Write-Host "[8/10] Validating package contents and architecture..." -ForegroundColor Cyan
    $requiredPaths = @(
        "LASSIE.exe",
        "CMOD.exe",
        "Qt6Core.dll",
        "Qt6Gui.dll",
        "Qt6Widgets.dll",
        "platforms\qwindows.dll",
        "msvcp140.dll",
        "msvcp140_1.dll",
        "vcruntime140.dll",
        "vcruntime140_1.dll",
        "Run-DISSCO.bat",
        "README.txt",
        "LICENSE"
    )
    $missing = @($requiredPaths | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $StageRoot $_) -PathType Leaf)
    })
    if (-not (Test-Path -LiteralPath $BundledLilyPondExe -PathType Leaf)) {
        $missing += $BundledLilyPondExe
    }
    if ($missing.Count -gt 0) {
        throw "Portable package validation failed. Missing: $($missing -join ', ')"
    }

    foreach ($peFile in @(
        (Join-Path $StageRoot "LASSIE.exe"),
        (Join-Path $StageRoot "CMOD.exe"),
        (Join-Path $StageRoot "Qt6Core.dll")
    )) {
        if ((Get-PeMachine -Path $peFile) -ne 0x8664) {
            throw "The portable package contains a non-x64 binary: $peFile"
        }
    }

    foreach ($sourceAndPackaged in @(
        @($LassieExe.FullName, (Join-Path $StageRoot "LASSIE.exe")),
        @($CmodExe.FullName, (Join-Path $StageRoot "CMOD.exe"))
    )) {
        $sourceHash = (Get-FileHash -LiteralPath $sourceAndPackaged[0] -Algorithm SHA256).Hash
        $packagedHash = (Get-FileHash -LiteralPath $sourceAndPackaged[1] -Algorithm SHA256).Hash
        if ($sourceHash -ne $packagedHash) {
            throw "Packaged executable differs from the current build: $($sourceAndPackaged[1])"
        }
    }

    if (-not $SkipSmokeTest) {
        Write-Host "[9/10] Testing without developer Qt/MSVC/LilyPond paths..." -ForegroundColor Cyan
        Invoke-PortableSmokeTests `
            -PackageRoot $StageRoot `
            -BundledLilyPondExe $BundledLilyPondExe
    }
    else {
        Write-Host "[9/10] Portable smoke tests skipped by request." -ForegroundColor DarkGray
    }

    Write-Host "[10/10] Publishing the portable folder and ZIP..." -ForegroundColor Cyan
    if (Test-Path -LiteralPath $PackageRoot) {
        Remove-Item -LiteralPath $PackageRoot -Recurse -Force
    }
    Move-Item -LiteralPath $StageRoot -Destination $PackageRoot

    if (-not $NoZip) {
        if (Test-Path -LiteralPath $ZipPath) {
            Remove-Item -LiteralPath $ZipPath -Force
        }
        if (Test-Path -LiteralPath $ChecksumPath) {
            Remove-Item -LiteralPath $ChecksumPath -Force
        }
        Compress-Archive `
            -Path $PackageRoot `
            -DestinationPath $ZipPath `
            -CompressionLevel Optimal
        $zipHash = (Get-FileHash -LiteralPath $ZipPath -Algorithm SHA256).Hash.ToLowerInvariant()
        Set-Content `
            -LiteralPath $ChecksumPath `
            -Value "$zipHash  $([System.IO.Path]::GetFileName($ZipPath))" `
            -Encoding ASCII
    }

    $packageFiles = @(Get-ChildItem -LiteralPath $PackageRoot -Recurse -File)
    $packageBytes = ($packageFiles | Measure-Object -Property Length -Sum).Sum
    Write-Host ""
    Write-Host "Portable package created successfully." -ForegroundColor Green
    Write-Host "Folder: $PackageRoot"
    Write-Host ("Files:  {0:N0}" -f $packageFiles.Count)
    Write-Host ("Size:   {0:N1} MB" -f ($packageBytes / 1MB))
    if (-not $NoZip) {
        Write-Host "ZIP:    $ZipPath"
        Write-Host "SHA256: $zipHash"
    }
}
finally {
    if (Test-Path -LiteralPath $StageRoot) {
        Assert-DirectChildPath -Parent $DistRoot -Child $StageRoot
        Remove-Item -LiteralPath $StageRoot -Recurse -Force
    }
}
