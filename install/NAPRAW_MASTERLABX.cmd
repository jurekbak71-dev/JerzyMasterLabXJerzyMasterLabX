@echo off
setlocal
cd /d "%~dp0"
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -ExecutionPolicy Bypass -File "%~dp0NAPRAW_MASTERLABX.ps1"
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" (echo REPAIR FAILED.) else (echo REPAIR SUCCESS: hash checked.)
pause
exit /b %ERR%
