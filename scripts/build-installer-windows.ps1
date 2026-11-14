<#
.SYNOPSIS
  Build dist\GrooveForgeBass-<version>-win64-setup.exe with Inno Setup 6 (after build-windows.ps1).
  Installs Inno Setup via Chocolatey if ISCC.exe is not found.
#>
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$version = (Select-String -Path "CMakeLists.txt" -Pattern 'project\(GrooveForgeBass VERSION ([0-9.]+)').Matches[0].Groups[1].Value
$art = Join-Path $root "build\windows-x64\GrooveForgeBass_artefacts\Release"
if (-not (Test-Path (Join-Path $art "VST3\GrooveForge Bass.vst3"))) { throw "Build first: $art" }

$iscc = (Get-Command iscc.exe -ErrorAction SilentlyContinue).Source
if (-not $iscc) { $iscc = @("${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1 }
if (-not $iscc) {
    choco install innosetup -y --no-progress | Out-Host
    $iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
}
& $iscc "/DAppVersion=$version" "/DBuildDir=$art" "installer\windows\GrooveForgeBass.iss"
if ($LASTEXITCODE -ne 0) { throw "ISCC failed ($LASTEXITCODE)" }
$exe = Join-Path $root "dist\GrooveForgeBass-$version-win64-setup.exe"
$hash = (Get-FileHash -Algorithm SHA256 $exe).Hash.ToLower()
"$hash  $(Split-Path -Leaf $exe)" | Out-File -Encoding ascii "$exe.sha256"
Write-Host "Installer: $exe"
Write-Host "SHA-256  : $hash"
