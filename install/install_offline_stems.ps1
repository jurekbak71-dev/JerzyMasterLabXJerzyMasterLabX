param([switch]$AcceptExternalDownloads)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$version='v0.1.6'
$archive='stems-v0.1.6-windows-x64-core.zip'
$runtimeBase='https://github.com/betweentwomidnights/stems.cpp/releases/download/'+$version
$modelFile='htdemucs-42M-v1.0-F16.gguf'
$modelBase='https://huggingface.co/thepatch/htdemucs-GGUF/resolve/main/'
if (-not $AcceptExternalDownloads) {
 Write-Host 'OPTIONAL OFFLINE ENGINE: stems.cpp HTDemucs, not the LIVE StemgenRT model.'
 Write-Host 'Downloads the upstream Windows x64 runtime and approx. 100 MiB model.'
 Write-Host 'Sources: https://github.com/betweentwomidnights/stems.cpp/releases'
 Write-Host '         https://huggingface.co/thepatch/htdemucs-GGUF'
 $answer=Read-Host 'Type INSTALL to download third-party engine and model'
 if ($answer -cne 'INSTALL'){throw 'No files changed.'}
}
$target=Join-Path $env:ProgramData 'JerzyMasterLabX\StemEngine'
$stage=Join-Path $env:TEMP ('MasterLabX-Stems-'+[guid]::NewGuid().ToString('N'))
function Verify-Manifest([string]$manifest,[string]$file,[string]$name) {
 $line=@(Get-Content -LiteralPath $manifest | Where-Object {
  $_ -match '^[a-fA-F0-9]{64}\s+\*?' -and $_.Trim().EndsWith($name)
 } | Select-Object -First 1)
 if($line.Count -ne 1){throw 'Missing SHA256 in upstream manifest: '+$name}
 $expected=($line[0] -split '\s+')[0].ToLowerInvariant()
 $actual=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant()
 if($expected -ne $actual){throw 'SHA256 mismatch for '+$name}
 Write-Host ('SHA256 verified: '+$name+' '+$actual)
}
New-Item -ItemType Directory -Force $stage | Out-Null
try {
 $checks=Join-Path $stage 'runtime-SHA256SUMS'
 $zip=Join-Path $stage $archive
 Invoke-WebRequest -Uri ($runtimeBase+'/SHA256SUMS') -OutFile $checks -UseBasicParsing
 Invoke-WebRequest -Uri ($runtimeBase+'/'+$archive) -OutFile $zip -UseBasicParsing
 Verify-Manifest $checks $zip $archive
 $unpack=Join-Path $stage 'unpacked'
 Expand-Archive -LiteralPath $zip -DestinationPath $unpack
 $exe=Get-ChildItem -LiteralPath $unpack -Filter 'stems-split.exe' -Recurse -File | Select-Object -First 1
 if(-not $exe){throw 'Official release has no stems-split.exe'}
 $modelChecks=Join-Path $stage 'model-SHA256SUMS'
 $model=Join-Path $stage $modelFile
 Invoke-WebRequest -Uri ($modelBase+'SHA256SUMS') -OutFile $modelChecks -UseBasicParsing
 Invoke-WebRequest -Uri ($modelBase+$modelFile) -OutFile $model -UseBasicParsing
 Verify-Manifest $modelChecks $model $modelFile
 $new=Join-Path $stage 'new'
 New-Item -ItemType Directory -Force (Join-Path $new 'models') | Out-Null
 Copy-Item -LiteralPath (Join-Path $exe.DirectoryName '*') -Destination $new -Recurse -Force
 Copy-Item -LiteralPath $model -Destination (Join-Path $new 'models' $modelFile) -Force
 # Install into ProgramData only after both upstream artifacts are verified.
 $parent=Split-Path -Parent $target
 New-Item -ItemType Directory -Force $parent | Out-Null
 $backup=$target+'-backup-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
 if(Test-Path -LiteralPath $target){Move-Item -LiteralPath $target -Destination $backup}
 try{Move-Item -LiteralPath $new -Destination $target}
 catch{
  if(Test-Path -LiteralPath $backup){Move-Item -LiteralPath $backup -Destination $target}
  throw
 }
 if(-not (Test-Path -LiteralPath (Join-Path $target 'stems-split.exe'))){
  throw 'Installed engine is missing stems-split.exe'
 }
 Write-Host ('OFFLINE ENGINE INSTALLED: '+$target) -ForegroundColor Green
 Write-Host 'Restart MasterLabX, select SEPARATE WAV, then choose an input WAV.'
} finally {
 Remove-Item -LiteralPath $stage -Recurse -Force -ErrorAction SilentlyContinue
}
