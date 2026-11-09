@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install_offline_stems.ps1"
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" echo OFFLINE ENGINE INSTALL FAILED. No success claimed.
pause
exit /b %ERR%
