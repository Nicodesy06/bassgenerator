<#
.SYNOPSIS
  Install the VST3 into the system VST3 folder every host scans:
  C:\Program Files\Common Files\VST3\GrooveForge Bass.vst3
  Re-launches itself elevated (UAC prompt) when not run as administrator.
  Also removes a stale per-user copy left by 0.1/0.2
  (%LOCALAPPDATA%\Programs\Common\VST3), which most DAWs never scanned.
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
$Bundle = (Resolve-Path $Bundle).Path

$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) {
    Start-Process powershell -Verb RunAs -Wait -ArgumentList @('-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-Bundle', "`"$Bundle`"")
    exit
}

$dest = Join-Path $env:CommonProgramFiles "VST3"
New-Item -ItemType Directory -Force -Path $dest | Out-Null
$target = Join-Path $dest "GrooveForge Bass.vst3"
if (Test-Path $target) { Remove-Item -Recurse -Force $target }
Copy-Item -Recurse $Bundle $target
$old = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3\GrooveForge Bass.vst3"
if (Test-Path $old) { Remove-Item -Recurse -Force $old; Write-Host "Removed old per-user copy: $old" }
Write-Host "Installed: $target"
Write-Host "Rescan plug-ins in your DAW (Ableton: enable 'Use VST3 Plug-In System Folders')."
Read-Host "Press Enter to close"
