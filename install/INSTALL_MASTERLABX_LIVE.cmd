@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install_masterlabx_live.ps1"
if errorlevel 1 (
 echo LIVE installation failed. Previous VST3 was not deliberately removed.
 pause
 exit /b 1
)
echo Preview installed. Rescan VST3 in FL Studio, set 44.1 kHz and enable LIVE NEURAL.
pause
