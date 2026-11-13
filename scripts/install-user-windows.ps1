<#
.SYNOPSIS
  Install the VST3 for the current user only (no administrator rights):
  %LOCALAPPDATA%\Programs\Common\VST3\GrooveForge Bass.vst3
  For all users copy the bundle to C:\Program Files\Common Files\VST3\ instead (requires admin).
#>
param([string]$Bundle = "")
$ErrorActionPreference = "Stop"
if (-not $Bundle) {
    $candidates = @(
        (Join-Path $PSScriptRoot "GrooveForge Bass.vst3"),
        (Join-Path (Split-Path -Parent $PSScriptRoot) "build\windows-x64\GrooveForgeBass_artefacts\Release\VST3\GrooveForge Bass.vst3"))
    $Bundle = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $Bundle -or -not (Test-Path $Bundle)) { throw "GrooveForge Bass.vst3 not found. Pass -Bundle <path>." }
$dest = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
$target = Join-Path $dest "GrooveForge Bass.vst3"
if (Test-Path $target) { Remove-Item -Recurse -Force $target }
Copy-Item -Recurse $Bundle $target
Write-Host "Installed: $target"
Write-Host "Rescan plug-ins in your DAW (REAPER: Options > Preferences > Plug-ins > VST > Re-scan)."
