<#
.SYNOPSIS
  Create dist\GrooveForgeBass-<version>-win64.zip with the VST3 bundle, the Standalone exe,
  README, licence notices, quality report and a SHA-256 checksum file.
#>
param([string]$Preset = "windows-x64-release")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$version = (Select-String -Path "CMakeLists.txt" -Pattern 'project\(GrooveForgeBass VERSION ([0-9.]+)').Matches[0].Groups[1].Value
$art = "build\windows-x64\GrooveForgeBass_artefacts\Release"
$vst3 = Join-Path $art "VST3\GrooveForge Bass.vst3"
$exe = Join-Path $art "Standalone\GrooveForge Bass.exe"
if (-not (Test-Path $vst3)) { throw "Build first: $vst3 missing" }

$name = "GrooveForgeBass-$version-win64"
$stage = Join-Path "dist" $name
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -Recurse $vst3 (Join-Path $stage "GrooveForge Bass.vst3")
if (Test-Path $exe) { Copy-Item $exe $stage }
Copy-Item README.md, LICENSE-THIRD-PARTY.md $stage
Copy-Item docs\QUALITY_REPORT.md, docs\DAW_COMPATIBILITY.md $stage
Copy-Item scripts\install-user-windows.ps1 $stage

$zip = Join-Path "dist" "$name.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path "$stage\*" -DestinationPath $zip
$hash = (Get-FileHash -Algorithm SHA256 $zip).Hash.ToLower()
"$hash  $name.zip" | Out-File -Encoding ascii (Join-Path "dist" "$name.zip.sha256")
Write-Host "Package : $((Resolve-Path $zip).Path)"
Write-Host "SHA-256 : $hash"
