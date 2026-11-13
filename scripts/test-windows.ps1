<#
.SYNOPSIS
  Run all automated tests (CTest), generator diagnostics and, if available, pluginval.
.PARAMETER Pluginval
  Optional path to pluginval.exe. If omitted, pluginval on PATH is used when present; otherwise it is skipped (reported).
#>
param(
    [string]$Preset = "windows-x64-release",
    [string]$Pluginval = "",
    [int]$Strictness = 10
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
New-Item -ItemType Directory -Force -Path "artifacts\logs", "artifacts\reports" | Out-Null

Write-Host "== CTest"
ctest --preset $Preset 2>&1 | Tee-Object -FilePath "artifacts\logs\ctest.log"
$ctestExit = $LASTEXITCODE

Write-Host "== Generator diagnostics"
& "build\windows-x64\tools\Release\pattern_diagnostics.exe" --count 200 --out "artifacts\reports" | Out-File "artifacts\logs\diagnostics.log"
$diagExit = $LASTEXITCODE
& "build\windows-x64\tools\Release\render_demo.exe" --out "artifacts\reports\demos" | Out-File "artifacts\logs\render_demo.log"

$vst3 = Join-Path $root "build\windows-x64\GrooveForgeBass_artefacts\Release\VST3\GrooveForge Bass.vst3"
if (-not $Pluginval) { $cmd = Get-Command pluginval.exe -ErrorAction SilentlyContinue; if ($cmd) { $Pluginval = $cmd.Source } }
$pvExit = 0
if ($Pluginval -and (Test-Path $Pluginval)) {
    Write-Host "== pluginval (strictness $Strictness): $Pluginval"
    & $Pluginval --strictness-level $Strictness --validate-in-process --output-dir "artifacts\pluginval" $vst3 2>&1 | Tee-Object -FilePath "artifacts\logs\pluginval.log"
    $pvExit = $LASTEXITCODE
} else {
    Write-Host "pluginval not found: SKIPPED (not run). Pass -Pluginval C:\path\pluginval.exe to run it." | Tee-Object -FilePath "artifacts\logs\pluginval.log"
}

Write-Host "ctest=$ctestExit diagnostics=$diagExit pluginval=$pvExit"
if ($ctestExit -ne 0 -or $diagExit -ne 0 -or $pvExit -ne 0) { exit 1 }
