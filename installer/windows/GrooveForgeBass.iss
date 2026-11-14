; Inno Setup 6 script for the GrooveForge Bass Windows installer (.exe).
; Build: ISCC.exe /DAppVersion=0.1.0 /DBuildDir=<...>\GrooveForgeBass_artefacts\Release installer\windows\GrooveForgeBass.iss
; Admin install  -> VST3 in C:\Program Files\Common Files\VST3, app in Program Files
; Per-user install (no admin, chosen in the first dialog) -> VST3 in %LOCALAPPDATA%\Programs\Common\VST3

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
PrivilegesRequiredOverridesAllowed=dialog
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

[Files]
Source: "{#BuildDir}\VST3\{#AppName}.vst3\*"; DestDir: "{code:Vst3Dir}\{#AppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\LICENSE-THIRD-PARTY.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "{cm:LaunchProgram,{#AppName}}"; Components: standalone; Flags: nowait postinstall skipifsilent unchecked

[Code]
function Vst3Dir(Param: String): String;
begin
  if IsAdminInstallMode then
    Result := ExpandConstant('{commoncf64}\VST3')
  else
    Result := ExpandConstant('{localappdata}\Programs\Common\VST3');
end;
