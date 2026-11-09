@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install_masterlabx.ps1"
if errorlevel 1 (
  echo Installation failed. No project presets were deleted.
  pause
  exit /b 1
)
echo.
echo Ready: scan VST3 plugins in FL Studio and open Stem Brain.
pause
