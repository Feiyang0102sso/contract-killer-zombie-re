#!/bin/bash
# Compile one CKZ source file with the fixed M1 toolchain (called by the make rules of build_ckz.sh).
# Usage: ckz_compile_one.sh <relative source> <object base name>
#
# This file is the only place the compile flags are defined; build_ckz.sh hashes it, so any change
# here rebuilds everything. Writes /work/ckz_build/obj/<base>.o (via a temporary file, so an
# interrupted compile never leaves a half-written object that make would treat as up to date),
# /work/ckz_build/dep/<base>.d and /work/ckz_build/log/<base>.log.
set -uo pipefail

RELATIVE=$1
BASE=$2

ROOT=/Applications/buildAgent/work/c426877c688cbe05
SDK=/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk
# b109: Xcode 4.2.1's own compiler, Apple clang-211.12 (x86_64 Mach-O), run through maloader (patched, see
# ckz_repo/recovery/m3/notes/b109-toolchain-probe.md). It removes the vorr/vpop "toolchain" DIFFs of the
# rebuilt clang-211.10.1 (/work/build/clang-211.10.1, still used for llvm-mc and for comparison).
LOADER=/work/maloader_src/maloader/ld-mac
CLANG=/work/xcode421/packages/clang/usr/bin/clang
BUILD=/work/ckz_build
OUT=$BUILD/obj
DEPS=$BUILD/dep
LOGS=$BUILD/log
PREFIX_HEADER=$ROOT/zombsniper_Prefix.pch

# M1 flags (see ckz_repo/recovery/m1/M1_REPORT.md); language flags per file type below
# -fvisibility=hidden: Xcode "Symbols Hidden by Default"; original Glu symbols are private extern (N_PEXT),
# and weak vtables of inline-only classes are addressed directly instead of via the GOT (b007 finding)
COMMON_FLAGS="-arch armv7 -miphoneos-version-min=4.0 -isysroot $SDK -integrated-as -Os -g -fno-strict-aliasing -fvisibility=hidden"
CXX_FLAGS="-x c++ -fno-rtti -fno-exceptions"
OBJCXX_FLAGS="-x objective-c++ -fno-rtti"
OBJC_FLAGS="-x objective-c"
C_FLAGS="-x c"

case "$RELATIVE" in
    *.cpp) LANGUAGE="$CXX_FLAGS" ;;
    *.mm)  LANGUAGE="$OBJCXX_FLAGS" ;;
    *.m)   LANGUAGE="$OBJC_FLAGS" ;;
    *)     LANGUAGE="$C_FLAGS" ;;
esac

# l: the original TinyXML code units use libc / new[] directly, without the Glu NP prefix header.
# Their game consumers still use the project PCH.
PREFIX_FLAGS="-include $PREFIX_HEADER"
case "$RELATIVE" in
    src/tinyxml/tinystr.cpp|src/tinyxml/tinyxml.cpp|src/tinyxml/tinyxmlparser.cpp|src/tinyxml/tinyxmlerror.cpp)
        PREFIX_FLAGS="" ;;
esac

# Xcode header maps make every project header visible by name: -I for each header directory
# (computed once per build by build_ckz.sh)
INCLUDE_FLAGS=$(cat "$BUILD/include_flags")

TEMPORARY="$OUT/$BASE.o.tmp"
cd "$ROOT" || exit 1
# The driver would exec itself for -cc1, which maloader cannot do: ask it for the -cc1 command line
# with -### and run that line through maloader directly.
CC1_LINE=$($LOADER $CLANG $COMMON_FLAGS $LANGUAGE $PREFIX_FLAGS $INCLUDE_FLAGS -MMD -MF "$DEPS/$BASE.d" -MT "$OUT/$BASE.o" \
        -c "$ROOT/$RELATIVE" -o "$TEMPORARY" -### 2>&1 | grep '"-cc1"')
if [ -n "$CC1_LINE" ] && eval "$LOADER $CC1_LINE" > "$LOGS/$BASE.log" 2>&1; then
    mv -f "$TEMPORARY" "$OUT/$BASE.o"
    echo "ok   $RELATIVE"
else
    rm -f "$TEMPORARY"
    echo "FAIL $RELATIVE (log $LOGS/$BASE.log)"
    exit 1
fi
