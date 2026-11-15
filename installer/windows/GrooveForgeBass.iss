; Inno Setup 6 script for the GrooveForge Bass Windows installer (.exe).
; Build: ISCC.exe /DAppVersion=0.1.0 /DBuildDir=<...>\GrooveForgeBass_artefacts\Release installer\windows\GrooveForgeBass.iss
; Always an admin install: VST3 in C:\Program Files\Common Files\VST3 (the only
; folder every VST3 host scans by default), app in Program Files.
; 0.1/0.2 offered a per-user mode that put the plug-in in
; %LOCALAPPDATA%\Programs\Common\VST3, which most DAWs do not scan: that copy is
; removed on upgrade so hosts do not see two versions.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef BuildDir
  #define BuildDir "..\..\build\windows-x64\GrooveForgeBass_artefacts\Release"
#endif
#define AppName "GrooveForge Bass"

[Setup]
AppId={{6F1C2B7A-3D4E-4B8F-9A61-5C0D2E7F8B13}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=GrooveForge Audio
AppSupportURL=https://github.com/Nicodesy06/bassgenerator
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
UsePreviousPrivileges=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir=..\..\dist
OutputBaseFilename=GrooveForgeBass-{#AppVersion}-win64-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#AppName}.exe
UninstallDisplayName={#AppName} {#AppVersion}
InfoBeforeFile=info_before.txt
LicenseFile=..\..\LICENSE-THIRD-PARTY.md

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "{cm:FullInstall}"
Name: "custom"; Description: "{cm:CustomInstall}"; Flags: iscustom

[Components]
Name: "vst3"; Description: "{cm:CompVst3}"; Types: full custom
Name: "standalone"; Description: "{cm:CompStandalone}"; Types: full

[CustomMessages]
italian.FullInstall=Installazione completa
italian.CustomInstall=Installazione personalizzata
italian.CompVst3=Plug-in VST3 (strumento)
italian.CompStandalone=Applicazione standalone
english.FullInstall=Full installation
english.CustomInstall=Custom installation
english.CompVst3=VST3 plug-in (instrument)
english.CompStandalone=Standalone application

[InstallDelete]
; stale per-user copy from 0.1 / 0.2 "install for me only"
Type: filesandordirs; Name: "{localappdata}\Programs\Common\VST3\{#AppName}.vst3"

[Files]
Source: "{#BuildDir}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE-THIRD-PARTY.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "{cm:LaunchProgram,{#AppName}}"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[Messages]
italian.FinishedLabel=GrooveForge Bass e' installato.%n%nVST3: C:\Program Files\Common Files\VST3\GrooveForge Bass.vst3%n%nApri il DAW e ripeti la scansione dei plug-in. In Ableton Live: Impostazioni > Plug-in > attiva "Usa cartelle di sistema VST3" e premi "Rescan". In FL Studio: Options > Manage plugins > Find more plugins.
english.FinishedLabel=GrooveForge Bass is installed.%n%nVST3: C:\Program Files\Common Files\VST3\GrooveForge Bass.vst3%n%nOpen your DAW and rescan plug-ins. Ableton Live: Settings > Plug-Ins > enable "Use VST3 Plug-In System Folders" and press Rescan. FL Studio: Options > Manage plugins > Find more plugins.
