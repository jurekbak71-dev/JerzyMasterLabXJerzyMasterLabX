param([switch]$VerifyOnly)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$source = Split-Path -Parent $PSCommandPath
$bundle = Join-Path $source 'JerzyMasterLabX.vst3'
$binary = Join-Path $bundle 'Contents\x86_64-win\JerzyMasterLabX.vst3'
$manifest = Join-Path $source 'MASTERLABX_SHA256.txt'

# Validate the packaged binary BEFORE requesting administrator privileges,
# closing the DAW or modifying any installed plugin.
if (-not (Test-Path -LiteralPath $binary -PathType Leaf)) {
  throw 'VST3 binary missing. Extract the complete installer ZIP first.'
}
if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) {
  throw 'Missing MASTERLABX_SHA256.txt. Reject unverified installer.'
}
$expected = (Get-Content -LiteralPath $manifest -Raw).Trim().ToUpperInvariant()
if ($expected -notmatch '^[0-9A-F]{64}$') {
  throw 'Invalid SHA-256 manifest.'
}
$actual = (Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash.ToUpperInvariant()
if ($actual -ne $expected) {
  throw "VST3 SHA-256 mismatch. Expected $expected, found $actual. Installation aborted."
}
if ($VerifyOnly) {
  Write-Host "Verified Windows x64 VST3 SHA-256: $actual"
  exit 0
}
if (Get-Process -Name FL64,FL -ErrorAction SilentlyContinue) {
  throw 'Close FL Studio before replacing its loaded VST3.'
}

$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
  $argument = '-NoProfile -ExecutionPolicy Bypass -File "' + $PSCommandPath + '"'
  $process = Start-Process -FilePath 'powershell.exe' -Verb RunAs -Wait -PassThru -ArgumentList $argument
  exit $process.ExitCode
}

$pluginDir = Join-Path $env:CommonProgramFiles 'VST3'
New-Item -ItemType Directory -Force -Path $pluginDir | Out-Null
$destination = Join-Path $pluginDir 'JerzyMasterLabX.vst3'
$stage = Join-Path $pluginDir 'JerzyMasterLabX.vst3.installing'
$backup = Join-Path $pluginDir ('JerzyMasterLabX.vst3.backup-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
if (Test-Path -LiteralPath $stage) {
  throw "Previous staging directory exists: $stage. Inspect before retrying."
}
$hadExisting = Test-Path -LiteralPath $destination
if ($hadExisting) {
  Move-Item -LiteralPath $destination -Destination $backup -ErrorAction Stop
}
try {
  Copy-Item -LiteralPath $bundle -Destination $stage -Recurse -Force -ErrorAction Stop
  $installedBinary = Join-Path $stage 'Contents\x86_64-win\JerzyMasterLabX.vst3'
  if (-not (Test-Path -LiteralPath $installedBinary -PathType Leaf)) {
    throw 'Incomplete staged VST3 bundle.'
  }
  $copiedHash = (Get-FileHash -LiteralPath $installedBinary -Algorithm SHA256).Hash.ToUpperInvariant()
  if ($copiedHash -ne $expected) {
    throw 'Copy verification failed: VST3 SHA-256 mismatch.'
  }
  Move-Item -LiteralPath $stage -Destination $destination -ErrorAction Stop
}
catch {
  if (Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue
  }
  if ($hadExisting -and -not (Test-Path -LiteralPath $destination) -and
      (Test-Path -LiteralPath $backup)) {
    Move-Item -LiteralPath $backup -Destination $destination -ErrorAction Stop
  }
  throw
}
Write-Host "Installed verified VST3: $destination"
Write-Host "SHA-256: $actual"
if ($hadExisting) { Write-Host "Previous VST3 backup: $backup" }
Write-Host 'Rescan VST3 plugins in FL Studio.'
