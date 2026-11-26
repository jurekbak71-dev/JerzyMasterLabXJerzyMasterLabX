[Setup]
AppId=Jerzy.MasterLabPro.Development
AppName=Jerzy MasterLabPro (Development)
AppVersion=0.1.0
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\..\reports
OutputBaseFilename=MasterLabPro-C-development-Setup
Compression=lzma2
SolidCompression=yes
Uninstallable=yes

[Files]
Source: "..\..\build-masterlabpro\MasterLabPro_artefacts\Release\VST3\*"; DestDir: "{commoncf64}\VST3"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\assets\*"; DestDir: "{commonappdata}\Jerzy\MasterLabPro\assets"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\..\build-masterlabpro\Release\mlp_unity.dll"; DestDir: "{commonappdata}\Jerzy\MasterLabPro\modules"; Flags: ignoreversion
Source: "..\..\build-masterlabpro\Release\mlp_gain.dll"; DestDir: "{commonappdata}\Jerzy\MasterLabPro\modules"; Flags: ignoreversion
Source: "..\..\build-masterlabpro\Release\mlp_saturation.dll"; DestDir: "{commonappdata}\Jerzy\MasterLabPro\modules"; Flags: ignoreversion
Source: "..\..\build-masterlabpro\Release\mlp_delay.dll"; DestDir: "{commonappdata}\Jerzy\MasterLabPro\modules"; Flags: ignoreversion
Source: "..\..\build-masterlabpro\Release\mlp_z_input_compressor.dll"; DestDir: "{commonappdata}\Jerzy\MasterLabPro\modules"; Flags: ignoreversion
