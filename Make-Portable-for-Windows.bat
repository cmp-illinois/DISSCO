@echo off
setlocal
cd /d "%~dp0"

echo Building and validating the DISSCO portable package...
echo.
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass ^
  -File "%~dp0Make-Portable-for-Windows.ps1" %*

if errorlevel 1 (
  echo.
  echo Portable package creation FAILED.
  pause
  exit /b 1
)

echo.
echo Portable package creation completed.
echo See the dist folder for the portable directory and ZIP.
pause
