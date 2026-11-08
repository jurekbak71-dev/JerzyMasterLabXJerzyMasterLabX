param([switch]$AcceptNonCommercialTerms)
$ErrorActionPreference = 'Stop'
$source = Split-Path -Parent $PSCommandPath
$bundle = Join-Path $source 'JerzyMasterLabX.vst3'
$binary = Join-Path $bundle 'Contents\x86_64-win\JerzyMasterLabX.vst3'
$runtime = Join-Path $bundle 'Contents\x86_64-win\onnxruntime.dll'
$notice = Join-Path $source 'licenses\StemgenRT\WEIGHTS-LICENSE.md'
if (!(Test-Path $binary) -or !(Test-Path $runtime) -or !(Test-Path $notice)) {
  throw 'The complete LIVE VST3 package (including ONNX Runtime and license) must be extracted first.'
}
# The CI runner includes MSVC, but FL Studio users may not. The native VST3
# imports these DLLs before JUCE can display any useful diagnostic.
$vcDlls = @('MSVCP140.dll','MSVCP140_1.dll','MSVCP140_ATOMIC_WAIT.dll','VCRUNTIME140.dll','VCRUNTIME140_1.dll')
$missingVc = @($vcDlls | Where-Object { !(Test-Path -LiteralPath (Join-Path $env:WINDIR ('System32\\' + $_)) -PathType Leaf) })
if ($missingVc.Count -gt 0) {
  Write-Host ('Required x64 Microsoft Visual C++ runtime files missing: ' + ($missingVc -join ', ')) -ForegroundColor Red
  Write-Host 'Install the official Microsoft Visual C++ Redistributable (x64) and run this installer again.'
  Write-Host 'https://aka.ms/vc14/vc_redist.x64.exe'
  throw 'Cannot install LIVE preview until required Windows runtime libraries are installed.'
}
Write-Host 'MasterLabX LIVE PREVIEW: personal NONCOMMERCIAL evaluation.'
Write-Host 'StemgenRT v0.7/0.8 weights: CC BY-NC-SA 4.0. Upstream does not guarantee third-party dataset clearance.'
Write-Host 'See licenses\StemgenRT\WEIGHTS-LICENSE.md and MODEL_CARD.md before proceeding.'
if (!$AcceptNonCommercialTerms) {
  $answer = Read-Host 'Type ACCEPT to approve noncommercial evaluation terms and download pinned weights'
  if ($answer -cne 'ACCEPT') { throw 'No changes made: terms not accepted.' }
}
$principal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
  $arguments = '-NoProfile -ExecutionPolicy Bypass -File "' + $PSCommandPath + '" -AcceptNonCommercialTerms'
  $elevated = Start-Process powershell.exe -Verb RunAs -Wait -PassThru -ArgumentList $arguments
  exit $elevated.ExitCode
}
$revision = '7d2d0c19d956f67259b8358c4127f991077e02b0'
$models = @(
  @{name='drums.onnx';path='model/parallel/drums.onnx';size=12267886;sha='f2a3fc5dd90f45690536370f02d9b9377e6625ab08cb32b3e921371ad134d874'},
  @{name='bass.onnx';path='model/parallel/bass.onnx';size=8660860;sha='1187b414f22344028035caba0a763d2211d13fe8bfce366c5f6b7ee4af77df40'},
  @{name='vocals.onnx';path='model/parallel/vocals.onnx';size=11019332;sha='f29a3d8fa596ce4672dd4dc77ccf224f2538389940264aad46ab8168311c2148'}
)
$stage = Join-Path $env:TEMP ('masterlab-live-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $stage | Out-Null
try {
  foreach ($model in $models) {
    $url = 'https://media.githubusercontent.com/media/sweetspotsoundsystem/stemgen-rt/' + $revision + '/' + $model.path
    $target = Join-Path $stage $model.name
    Write-Host ('Downloading model: ' + $model.name)
    Invoke-WebRequest -Uri $url -OutFile $target -UseBasicParsing
    $hash = (Get-FileHash $target -Algorithm SHA256).Hash.ToLowerInvariant()
    if ((Get-Item $target).Length -ne $model.size -or $hash -ne $model.sha) {
      throw ('Model size or SHA-256 mismatch: ' + $model.name)
    }
  }
  $programData = Join-Path $env:ProgramData 'JerzyMasterLabX'
  $targetModels = Join-Path $programData 'StemgenRT'
  $targetPlugin = Join-Path $env:CommonProgramFiles 'VST3\JerzyMasterLabX.vst3'
  $backup = Join-Path $programData ('Backup-Live-' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
  New-Item -ItemType Directory -Force $targetModels,$backup | Out-Null
  $old = Join-Path $backup 'JerzyMasterLabX.vst3'
  if (Test-Path $targetPlugin) { Copy-Item $targetPlugin $old -Recurse -Force }
  foreach ($model in $models) {
    Copy-Item (Join-Path $stage $model.name) (Join-Path $targetModels $model.name) -Force
  }
  try {
    if (Test-Path $targetPlugin) { Remove-Item $targetPlugin -Recurse -Force }
    Copy-Item $bundle $targetPlugin -Recurse -Force
    if (!(Test-Path (Join-Path $targetPlugin 'Contents\x86_64-win\onnxruntime.dll'))) {
      throw 'ONNX Runtime DLL missing after copying VST3.'
    }
  } catch {
    if (Test-Path $targetPlugin) { Remove-Item $targetPlugin -Recurse -Force }
    if (Test-Path $old) { Copy-Item $old $targetPlugin -Recurse -Force }
    throw
  }
  Write-Host ('LIVE VST3 installed: ' + $targetPlugin)
  Write-Host ('Models verified and installed: ' + $targetModels)
  Write-Host ('Backup: ' + $backup)
  Write-Host 'Use 44.1 kHz in FL Studio; open Stem Brain and enable LIVE NEURAL.'
  Write-Host 'Previously installed offline StemEngine has not been deleted.'
} finally {
  Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
}
