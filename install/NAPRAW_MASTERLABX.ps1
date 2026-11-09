$ErrorActionPreference = 'Stop'
$source = Split-Path -Parent $PSCommandPath
$bundle = Join-Path $source 'JerzyMasterLabX.vst3'
$srcBinary = Join-Path $bundle 'Contents\x86_64-win\JerzyMasterLabX.vst3'
$target = Join-Path $env:CommonProgramFiles 'VST3\JerzyMasterLabX.vst3'
$targetBin = Join-Path $target 'Contents\x86_64-win\JerzyMasterLabX.vst3'
function HashOf([string] $path) {
 if (!(Test-Path -LiteralPath $path -PathType Leaf)) { return 'MISSING' }
 return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToUpperInvariant()
}
# Use the source ZIP as the source of truth. CI-generated manifest binds bytes
# to the exact tested build; fallback to package hashes for manual rebuilds.
$sourceBinHash = HashOf $srcBinary
if ($sourceBinHash -eq 'MISSING')
 { throw 'The full VST3 package must be unpacked first.' }
$manifest = Join-Path $source 'VST3-SHA256.txt'
if (Test-Path -LiteralPath $manifest) {
 $expected = (Get-Content -LiteralPath $manifest -TotalCount 1).Trim().ToUpperInvariant()
 if ($expected -ne $sourceBinHash) { throw 'The supplied VST3 does not match its package manifest.' }
}
Write-Host ('Package VST3 SHA256: ' + $sourceBinHash)
Write-Host ('Installed VST3 SHA256 before repair: ' + (HashOf $targetBin))
if (Get-Process -Name FL64 -ErrorAction SilentlyContinue)
 { throw 'Close FL Studio completely before repairing the VST3.' }
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
 $args = '-NoProfile -ExecutionPolicy Bypass -File "' + $PSCommandPath + '"'
 $p = Start-Process powershell.exe -Verb RunAs -ArgumentList $args -Wait -PassThru
 exit $p.ExitCode
}
$backupDir = Join-Path (Join-Path $env:ProgramData 'JerzyMasterLabX') ('Backup-Verified-Repair-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $backupDir | Out-Null
$backup = Join-Path $backupDir 'JerzyMasterLabX.vst3'
$hadOld = Test-Path -LiteralPath $target
if ($hadOld) { Copy-Item -LiteralPath $target -Destination $backup -Recurse -Force }
try {
 if ($hadOld) { Remove-Item -LiteralPath $target -Recurse -Force }
 Copy-Item -LiteralPath $bundle -Destination $target -Recurse -Force
 $actualBin = HashOf $targetBin
 if ($actualBin -ne $sourceBinHash)
   { throw ('Installed VST3 hash mismatch: ' + $actualBin) }
} catch {
 if (Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
 if ($hadOld -and (Test-Path -LiteralPath $backup))
   { Copy-Item -LiteralPath $backup -Destination $target -Recurse -Force }
 throw
}
Write-Host ('SUCCESS: installed VST3 SHA256 verified: ' + (HashOf $targetBin)) -ForegroundColor Green
Write-Host ('Backup: ' + $backupDir)
Write-Host 'Reopen FL Studio and rescan the VST3.'
