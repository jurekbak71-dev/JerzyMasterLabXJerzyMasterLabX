$ErrorActionPreference = "Stop"
$source = Split-Path -Parent $PSCommandPath
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
  Write-Host "MasterLabX needs elevation only to copy VST3 and StemEngine into standard program directories."
  $cmd = "-NoProfile -ExecutionPolicy Bypass -File " + [char]34 + $PSCommandPath + [char]34
  $process = Start-Process -FilePath "powershell.exe" -Verb RunAs -Wait -PassThru -ArgumentList $cmd
  exit $process.ExitCode
}
$bundle = Join-Path $source "JerzyMasterLabX.vst3"
$native = Join-Path $source "StemEngine"
if (-not (Test-Path (Join-Path $bundle "Contents\x86_64-win\JerzyMasterLabX.vst3"))) {
  throw "Windows x64 VST3 file is missing or ZIP was not extracted completely."
}
if (-not (Test-Path (Join-Path $native "stems-split.exe"))) {
  throw "Native HTDemucs CPU separator is missing from the installation package."
}
if (-not (Test-Path (Join-Path $native "models\htdemucs-42M-v1.0-F16.gguf"))) {
  throw "HTDemucs model is missing from the installation package."
}
$plugindir = Join-Path $env:CommonProgramFiles "VST3"
$enginedir = Join-Path $env:ProgramData "JerzyMasterLabX\StemEngine"
New-Item -Type Directory -Force $plugindir | Out-Null
New-Item -Type Directory -Force (Split-Path $enginedir -Parent) | Out-Null
$destination = Join-Path $plugindir "JerzyMasterLabX.vst3"
if (Test-Path $destination) { Remove-Item $destination -Recurse -Force }
Copy-Item $bundle -Destination $destination -Recurse -Force
if (Test-Path $enginedir) { Remove-Item $enginedir -Recurse -Force }
Copy-Item $native -Destination $enginedir -Recurse -Force
Write-Host "VST3 installed: $destination"
Write-Host "HTDemucs model/runtime installed: $enginedir"
Write-Host "Stems are processed OFFLINE locally; no audio is uploaded."
