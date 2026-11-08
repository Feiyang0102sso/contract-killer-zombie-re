#!/bin/bash
# One-time toolchain setup inside the build Linux (Ubuntu 24.04 x86_64, run as root).
# On Windows, setup.bat creates the WSL distro and calls this script; on a native Ubuntu run it with sudo.
#
# Usage: setup_distro.sh [xcode-dmg]
#   xcode-dmg  Xcode 4.2.1 (4D502) installer, default toolchain/xcode/installxcode_421_lion.dmg
#
# Steps (each one is skipped when its result already exists, so re-running is cheap):
#   1. apt packages
#   2. maloader (runs Apple's x86_64 Mach-O tools on Linux) at a fixed commit + our 3 fixes
#   3. the 4 Xcode component packages we need: clang-211.12, iOS 5.0 SDK, ld64-127.2, libLTO
#   4. ld64_root: ld next to ../lib/libLTO.dylib (ld loads it from there; the Xcode packages split them)
#   5. self check: run clang and ld through maloader
# All in-distro paths are the ones the build scripts use (build_ckz.sh, ckz_compile_one.sh, link_ckz.sh);
# they also end up in the debug info of the objects, so do not change them.
set -euo pipefail

TOOLCHAIN=$(cd "$(dirname "$0")" && pwd)
DMG=${1:-$TOOLCHAIN/xcode/installxcode_421_lion.dmg}

# ---- constants ----
XCODE_SHA256=a9bb263383fe102749ac4ba4d0bb02fba4dd13d5de5d71582b11a4acd52ef835
XCODE_DIR=/work/xcode421
XCODE_PACKAGES="clang iPhoneSDK5_0 iPhoneHostSideTools iPhoneSDKTools"
MALOADER_URL=https://github.com/shinh/maloader.git
MALOADER_COMMIT=464a90fdfd06a54c9da5d1a3725ed6229c0d3d60
MALOADER_DIR=/work/maloader_src/maloader
LOADER=$MALOADER_DIR/ld-mac
CLANG=$XCODE_DIR/packages/clang/usr/bin/clang
LD64_SOURCE=$XCODE_DIR/packages/iPhoneHostSideTools/Platforms/iPhoneOS.platform/Developer/usr/bin/ld
LTO_SOURCE=$XCODE_DIR/packages/iPhoneSDKTools/Platforms/iPhoneOS.platform/Developer/usr/lib/libLTO.dylib
LD64_ROOT=/work/ckz_build/ld64_root
# libssl-dev: maloader dlopens the unversioned libcrypto.so at run time (ld64 computes its UUID with MD5)
APT_PACKAGES="make g++ git ca-certificates rsync python3 7zip cpio uuid-dev zlib1g-dev libssl-dev"

if [ "$(id -u)" != 0 ]; then
    echo "setup_distro.sh: run as root (WSL: wsl -u root, Linux: sudo)" >&2
    exit 1
fi
if [ "$(uname -m)" != x86_64 ]; then
    echo "setup_distro.sh: maloader needs an x86_64 Linux, this is $(uname -m)" >&2
    exit 1
fi

echo "== 1/5 apt packages"
export DEBIAN_FRONTEND=noninteractive
apt-get update -q
apt-get install -y -q --no-install-recommends $APT_PACKAGES

echo "== 2/5 maloader $MALOADER_COMMIT"
if [ -x "$LOADER" ]; then
    echo "already built: $LOADER"
else
    rm -rf "$MALOADER_DIR"
    mkdir -p "$MALOADER_DIR"
    cd "$MALOADER_DIR"
    git init -q
    git fetch -q --depth 1 "$MALOADER_URL" "$MALOADER_COMMIT"
    git checkout -q FETCH_HEAD
    # b107 fixes: glibc >= 2.32 header, N_REF_TO_WEAK on undefined symbols, extrel binding
    git apply "$TOOLCHAIN/patches/maloader.patch"
    make release > build.log 2>&1 || { tail -30 build.log; echo "maloader build failed (log $MALOADER_DIR/build.log)"; exit 1; }
    echo "built: $LOADER"
fi

echo "== 3/5 Xcode 4.2.1 packages: $XCODE_PACKAGES"
missing=""
for package in $XCODE_PACKAGES; do
    if [ ! -d "$XCODE_DIR/packages/$package" ]; then
        missing="$missing $package"
    fi
done
if [ -z "$missing" ]; then
    echo "already extracted: $XCODE_DIR/packages"
else
    if [ ! -f "$DMG" ]; then
        echo "Xcode installer not found: $DMG" >&2
        echo "Download Xcode 4.2.1 for Lion (installxcode_421_lion.dmg, 4D502) from developer.apple.com" >&2
        echo "with your Apple ID and put it at toolchain/xcode/, or pass its path as the first argument." >&2
        exit 1
    fi
    echo "checking sha256 of $DMG"
    actual=$(sha256sum "$DMG" | cut -d' ' -f1)
    if [ "$actual" != "$XCODE_SHA256" ]; then
        echo "sha256 mismatch: expected $XCODE_SHA256, got $actual" >&2
        exit 1
    fi
    bash "$TOOLCHAIN/extract_xcode.sh" "$DMG" "$XCODE_DIR" $missing
fi

echo "== 4/5 ld64_root"
mkdir -p "$LD64_ROOT/usr/bin" "$LD64_ROOT/usr/lib"
cp -f "$LD64_SOURCE" "$LD64_ROOT/usr/bin/ld"
cp -f "$LTO_SOURCE" "$LD64_ROOT/usr/lib/libLTO.dylib"
chmod +x "$LD64_ROOT/usr/bin/ld" "$CLANG"

echo "== 5/5 self check"
# ld -v exits non-zero without inputs, so only the printed version is checked
clang_version=$("$LOADER" "$CLANG" --version 2>&1 || true)
ld_version=$("$LOADER" "$LD64_ROOT/usr/bin/ld" -v 2>&1 || true)
echo "$clang_version" | head -1
echo "$ld_version" | head -1
if ! echo "$clang_version" | grep -q 'clang-211.12'; then
    echo "self check failed: clang-211.12 does not run through maloader" >&2
    exit 1
fi
if ! echo "$ld_version" | grep -q 'ld64-127.2'; then
    echo "self check failed: ld64-127.2 does not run through maloader" >&2
    exit 1
fi
echo "setup done"
