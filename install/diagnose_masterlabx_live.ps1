param(
  [string]$PluginPath = (Join-Path $env:CommonProgramFiles 'VST3\JerzyMasterLabX.vst3'),
  [switch]$NoPause
)
$ErrorActionPreference = 'Stop'
$issues = New-Object 'System.Collections.Generic.List[string]'
$lines = New-Object 'System.Collections.Generic.List[string]'
function Log([string]$message) { Write-Host $message; [void]$lines.Add($message) }
function Issue([string]$message) { [void]$issues.Add($message); Log ('PROBLEM: ' + $message) }

Log 'MasterLabX LIVE - independent Windows loader diagnostic'
Log ('UTC: ' + [DateTime]::UtcNow.ToString('u'))
Log ('OS: ' + [Environment]::OSVersion.VersionString)
Log ('64-bit PowerShell: ' + [Environment]::Is64BitProcess)
Log ('Bundle: ' + $PluginPath)
if (-not [Environment]::Is64BitProcess) { Issue 'Use 64-bit PowerShell to test the 64-bit VST3.' }
$inner = Join-Path $PluginPath 'Contents\x86_64-win'
$binary = Join-Path $inner 'JerzyMasterLabX.vst3'
$ort = Join-Path $inner 'JerzyMasterLabX_ort_1_26.dll'
foreach ($file in @($binary, $ort)) {
  if (-not (Test-Path -LiteralPath $file -PathType Leaf)) {
    Issue ('Missing: ' + $file)
  } else {
    $f = Get-Item -LiteralPath $file
    $hash = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
    Log ('File: ' + $file + ' (' + $f.Length + ' bytes, SHA256=' + $hash + ')')
  }
}
# These are real imports of the Windows VST3/ONNX Runtime from the preview ZIP.
$required = @('MSVCP140.dll', 'MSVCP140_1.dll', 'MSVCP140_ATOMIC_WAIT.dll',
              'VCRUNTIME140.dll', 'VCRUNTIME140_1.dll')
foreach ($name in $required) {
  $path = Join-Path $env:WINDIR ('System32\' + $name)
  if (Test-Path -LiteralPath $path -PathType Leaf) {
    $v = (Get-Item -LiteralPath $path).VersionInfo.FileVersion
    Log ('Runtime OK: ' + $name + ' version=' + $v)
  } else {
    Issue ('Microsoft Visual C++ runtime missing: ' + $name)
  }
}
$modelDir = Join-Path $env:ProgramData 'JerzyMasterLabX\StemgenRT'
foreach ($name in @('drums.onnx','bass.onnx','vocals.onnx')) {
  $path = Join-Path $modelDir $name
  if (Test-Path -LiteralPath $path -PathType Leaf) { Log ('Model exists: ' + $name) }
  else { Log ('Model NOT installed: ' + $name + ' (LIVE mode unavailable; VST3 should still open)') }
}
if ([Environment]::Is64BitProcess -and (Test-Path -LiteralPath $binary -PathType Leaf)) {
  try {
    if (-not ('MasterLabXLoaderProbe' -as [type])) {
      Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class MasterLabXLoaderProbe {
 [DllImport("kernel32.dll", EntryPoint="LoadLibraryExW", CharSet=CharSet.Unicode, SetLastError=true)]
 public static extern IntPtr LoadLibraryEx(string file, IntPtr reserved, uint flags);
 [DllImport("kernel32.dll", SetLastError=true)]
 [return: MarshalAs(UnmanagedType.Bool)]
 public static extern bool FreeLibrary(IntPtr module);
}
'@
    }
    # Search beside this VST3 and normal Windows DLL directories (not arbitrary PATH).
    $flags = [uint32]0x00000100 -bor [uint32]0x00001000
    $module = [MasterLabXLoaderProbe]::LoadLibraryEx($binary, [IntPtr]::Zero, $flags)
    if ($module -eq [IntPtr]::Zero) {
      $errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
      Issue ('Windows failed to load VST3, Win32 error=' + $errorCode +
          ' (126=DLL missing; 193=wrong architecture; 1114=DLL initialization failure).')
    } else {
      Log 'LoadLibraryEx: OK. Windows resolved static DLL dependencies.'
      [void][MasterLabXLoaderProbe]::FreeLibrary($module)
      Log 'NOTE: this does not confirm FL Studio/JUCE plugin initialization.'
    }
  } catch {
    Issue ('LoadLibraryEx diagnostic exception: ' + $_.Exception.Message)
  }
}
# Windows Error Reporting gives the actual faulting DLL/offset for FL Studio.
# The Fruity Wrapper dialog alone cannot distinguish state, editor or worker crashes.
try {
  $since = (Get-Date).AddDays(-3)
  $events = @(Get-WinEvent -FilterHashtable @{LogName='Application';Id=1000,1001;StartTime=$since} -ErrorAction Stop |
    Where-Object { $_.ProviderName -in @('Application Error','Windows Error Reporting') -and
      $_.Message -match 'FL64\.exe|FL Studio|JerzyMasterLabX|onnxruntime' } |
    Select-Object -First 8)
  if($events.Count -eq 0) { Log 'No relevant Windows Application Error / WER events in the last 3 days.' }
  foreach($event in $events) {
    Log ('WER event time=' + $event.TimeCreated.ToString('u') + ' provider=' + $event.ProviderName + ' id=' + $event.Id)
    foreach($line in ($event.Message -split "[`r`n]+" | Where-Object {
      $_ -match '(?i)faulting application|faulting module|exception code|fault offset|nazwa aplikacji|nazwa modulu|kod wyjatku|przesuniecie bledu|JerzyMasterLabX|onnxruntime'
    } | Select-Object -First 16)){
      Log ('WER: ' + $line.Trim())
    }
  }
} catch {
  Log ('Windows Error Reporting query unavailable: ' + $_.Exception.Message)
}
if ($issues.Count -gt 0) {
 Log 'Review the problems above. Official x64 Visual C++ runtime: https://aka.ms/vc14/vc_redist.x64.exe'
} else {
 Log 'No missing imports discovered. FL Studio logs/diagnostics are needed to find a host initialization error.'
}
$report = Join-Path $env:TEMP ('MasterLabX-Diagnostic-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.txt')
$lines | Set-Content -LiteralPath $report -Encoding UTF8
Log ('Diagnostic report saved: ' + $report)
if (-not $NoPause) { [void](Read-Host 'Press Enter to close') }
if ($issues.Count -gt 0) { exit 1 }
exit 0
