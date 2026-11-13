<#
.SYNOPSIS
  Configure and build GrooveForge Bass (VST3 + Standalone + tests + tools) for Windows x64.
.PARAMETER JuceDir
  Optional path to a local JUCE 9.0.5 checkout (offline builds). Empty = CMake FetchContent (needs internet once).
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1
#>
param(
    [string]$JuceDir = $env:GFB_JUCE_DIR,
    [string]$Preset = "windows-x64-release"
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path "artifacts\logs" | Out-Null
if ($JuceDir) { $env:GFB_JUCE_DIR = (Resolve-Path $JuceDir).Path }

Write-Host "== Configure ($Preset)"
cmake --preset $Preset 2>&1 | Tee-Object -FilePath "artifacts\logs\configure.log"
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed (see artifacts\logs\configure.log)" }

Write-Host "== Build"
cmake --build --preset $Preset --parallel 2>&1 | Tee-Object -FilePath "artifacts\logs\build.log"
if ($LASTEXITCODE -ne 0) { throw "Build failed (see artifacts\logs\build.log)" }

$vst3 = Join-Path $root "build\windows-x64\GrooveForgeBass_artefacts\Release\VST3\GrooveForge Bass.vst3"
$exe  = Join-Path $root "build\windows-x64\GrooveForgeBass_artefacts\Release\Standalone\GrooveForge Bass.exe"
if (-not (Test-Path (Join-Path $vst3 "Contents\x86_64-win\GrooveForge Bass.vst3"))) { throw "VST3 bundle not found at $vst3" }
Write-Host "VST3 bundle : $vst3"
if (Test-Path $exe) { Write-Host "Standalone  : $exe" }
