@echo off
setlocal
cd /d "%~dp0"
%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0diagnose_masterlabx_live.ps1"
echo.
if errorlevel 1 (
  echo MasterLabX loader prerequisites require attention. See report path above.
) else (
  echo Loader probe completed. If FL Studio still fails, send the diagnostic report and FL Studio crash log.
)
pause
