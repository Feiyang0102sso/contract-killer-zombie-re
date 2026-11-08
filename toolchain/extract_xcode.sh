#!/bin/bash
# Unpack an Xcode 4.x installer dmg down to its component packages and their payloads.
# dmg (HFS+) -> InstallXcode*.pkg (xar) -> Payload (gzip cpio) -> Install Xcode.app/.../Packages/*.pkg (xar) -> Payload
# Usage: extract_xcode.sh <dmg> <out-dir> [package ...]
#   package  component package names to unpack (e.g. clang iPhoneSDK5_0); all of them when omitted
# Result: <out-dir>/packages/<name>/ holds each component package's payload tree.
# Needs 7z (Ubuntu package 7zip) and cpio. The temporary <out-dir>/stage is removed at the end.
set -euo pipefail

# 7z may already have un-gzipped the xar member (it then shows up as "Payload~"): unpack either form.
unpack_payload() {
    if gzip -t "$1" 2>/dev/null; then
        gzip -dc "$1" | cpio -id --quiet
    else
        cpio -id --quiet < "$1"
    fi
}

# true when the package was asked for (or when no filter was given)
wanted() {
    local name=$1
    local item
    if [ ${#WANTED_PACKAGES[@]} -eq 0 ]; then
        return 0
    fi
    for item in "${WANTED_PACKAGES[@]}"; do
        if [ "$item" = "$name" ]; then
            return 0
        fi
    done
    return 1
}

DMG="$1"
OUT="$2"
shift 2
WANTED_PACKAGES=("$@")
mkdir -p "$OUT/stage"
cd "$OUT/stage"

echo "step 1: dmg -> outer pkg"
7z x -y -bd "$DMG" '*/Install*.pkg' >/dev/null
OUTER_PKG=$(find . -maxdepth 3 -name 'Install*.pkg' -type f | head -1)
echo "  outer pkg: $OUTER_PKG"

echo "step 2: outer pkg (xar) -> payload"
mkdir -p outer
7z x -y -bd -oouter "$OUTER_PKG" >/dev/null
OUTER_PAYLOAD=$(find "$PWD/outer" -name 'Payload*' -type f | head -1)
mkdir -p outer_root
(cd outer_root && unpack_payload "$OUTER_PAYLOAD")

echo "step 3: component packages"
mkdir -p "$OUT/packages"
find outer_root -path '*Packages/*.pkg' -type f | sort | while read -r pkg; do
    name=$(basename "$pkg" .pkg)
    if [ -d "$OUT/packages/$name" ] || ! wanted "$name"; then
        continue
    fi
    tmp=$(mktemp -d)
    7z x -y -bd -o"$tmp" "$pkg" >/dev/null
    mkdir -p "$OUT/packages/$name"
    payload=$(find "$tmp" -name 'Payload*' -type f | head -1)
    if [ -n "$payload" ]; then
        (cd "$OUT/packages/$name" && unpack_payload "$payload") || echo "  WARN: payload of $name not cpio"
    fi
    rm -rf "$tmp"
    echo "  $name: $(du -sh "$OUT/packages/$name" | cut -f1)"
done

cd "$OUT"
rm -rf "$OUT/stage"
