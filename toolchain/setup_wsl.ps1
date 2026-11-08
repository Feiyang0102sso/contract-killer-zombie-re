# Create the build WSL distro inside this repository and install the toolchain (called by setup.bat).
# Usage: setup_wsl.ps1 [-XcodeDmg <path>] [-Distro <name>]
#   XcodeDmg  Xcode 4.2.1 installer, default toolchain\xcode\installxcode_421_lion.dmg
#   Distro    WSL distro name, default ckz-build
# The distro disk lives in <repo>\.wsl\<name>, the downloaded Ubuntu image in <repo>\.cache (both git-ignored).
param(
    [string]$XcodeDmg = "",
    [string]$Distro = "ckz-build"
)
$ErrorActionPreference = "Stop"
# make wsl.exe print UTF-8 instead of UTF-16
$env:WSL_UTF8 = "1"

# ---- constants ----
$RootfsBase = "https://cloud-images.ubuntu.com/wsl/releases/24.04/current"
$RootfsName = "ubuntu-noble-wsl-amd64-24.04lts.rootfs.tar.gz"

$Repo = Split-Path -Parent $PSScriptRoot
$CacheDir = Join-Path $Repo ".cache"
$DistroDir = Join-Path $Repo ".wsl\$Distro"
$RootfsPath = Join-Path $CacheDir $RootfsName
if ($XcodeDmg -eq "") {
    $XcodeDmg = Join-Path $Repo "toolchain\xcode\installxcode_421_lion.dmg"
}

# Fail early: the dmg cannot be downloaded automatically (Apple ID required)
if (-not (Test-Path $XcodeDmg)) {
    Write-Host "Xcode installer not found: $XcodeDmg"
    Write-Host "Download Xcode 4.2.1 for Lion (installxcode_421_lion.dmg) from developer.apple.com"
    Write-Host "and put it at toolchain\xcode\, or run: setup.bat <path to dmg>"
    exit 1
}
if (-not (Get-Command wsl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "wsl.exe not found. Install WSL first (admin PowerShell: wsl --install --no-distribution), then reboot."
    exit 1
}

# 1. distro: import the official Ubuntu 24.04 WSL image into the repository
# (wsl -l -q prints UTF-16; strip the NUL characters before comparing)
$existing = (wsl.exe -l -q) -replace "`0", "" | Where-Object { $_.Trim() -eq $Distro }
if ($existing) {
    Write-Host "== distro $Distro already exists"
} else {
    New-Item -ItemType Directory -Force $CacheDir | Out-Null
    if (-not (Test-Path $RootfsPath)) {
        Write-Host "== download $RootfsName (about 350 MB)"
        curl.exe -L --fail -o "$RootfsPath.part" "$RootfsBase/$RootfsName"
        if ($LASTEXITCODE -ne 0) { throw "download failed" }
        Move-Item -Force "$RootfsPath.part" $RootfsPath
    }
    Write-Host "== verify $RootfsName"
    $sums = (curl.exe -sL --fail "$RootfsBase/SHA256SUMS")
    if ($LASTEXITCODE -ne 0) { throw "cannot fetch SHA256SUMS" }
    $expected = ""
    foreach ($line in $sums) {
        if ($line -match "^([0-9a-f]{64})\s+\*?$([regex]::Escape($RootfsName))$") {
            $expected = $Matches[1]
        }
    }
    $actual = (Get-FileHash -Algorithm SHA256 $RootfsPath).Hash.ToLower()
    if ($expected -ne $actual) {
        # "current" may have moved on: drop the cached image so the next run downloads it again
        Remove-Item -Force $RootfsPath
        throw "sha256 mismatch for $RootfsName (expected '$expected', got $actual); run setup again"
    }
    Write-Host "== import distro $Distro -> $DistroDir"
    New-Item -ItemType Directory -Force $DistroDir | Out-Null
    wsl.exe --import $Distro $DistroDir $RootfsPath --version 2
    if ($LASTEXITCODE -ne 0) { throw "wsl --import failed" }
}

# 2. toolchain inside the distro (paths converted by wslpath)
$dmgLinux = (wsl.exe -d $Distro -u root --exec wslpath -a $XcodeDmg)
Write-Host "== toolchain setup in $Distro"
wsl.exe -d $Distro -u root --cd $Repo --exec bash toolchain/setup_distro.sh $dmgLinux
if ($LASTEXITCODE -ne 0) { throw "toolchain setup failed" }
Write-Host ""
Write-Host "Setup finished. Run build.bat to compile and link zombsniper."
