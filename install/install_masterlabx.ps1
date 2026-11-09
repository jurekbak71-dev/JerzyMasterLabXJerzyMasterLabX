$ErrorActionPreference = "Stop"
$source = Split-Path -Parent $PSCommandPath
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
  Write-Host "MasterLabX requires administrator rights to install VST3."
  $cmd = "-NoProfile -ExecutionPolicy Bypass -File " + [char]34 + $PSCommandPath + [char]34
  $process = Start-Process -FilePath "powershell.exe" -Verb RunAs -Wait -PassThru -ArgumentList $cmd
  exit $process.ExitCode
}
$bundle = Join-Path $source "JerzyMasterLabX.vst3"
if (-not (Test-Path (Join-Path $bundle "Contents\x86_64-win\JerzyMasterLabX.vst3"))) {
  throw "Windows x64 VST3 file is missing or ZIP was not extracted completely."
}
$plugindir = Join-Path $env:CommonProgramFiles "VST3"
New-Item -Type Directory -Force $plugindir | Out-Null
$destination = Join-Path $plugindir "JerzyMasterLabX.vst3"
if (Test-Path $destination) { Remove-Item $destination -Recurse -Force }
Copy-Item $bundle -Destination $destination -Recurse -Force
Write-Host "VST3 installed: $destination"
