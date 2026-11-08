#!/bin/bash
# Fast incremental build of the CKZ objects inside ckre (run as root).
# Usage: build_ckz.sh [-q] [-B] (--all | <rel_source>...)
#   rel_source  paths relative to CKZ_source (the original build root)
#   --all       every .c/.cpp/.m/.mm plus the Swerve library; removes objects of deleted sources
#   -q          only print failures and the summary
#   -B          rebuild the requested objects even if they are up to date
#
# What it does (b106):
#   1. rsync CKZ_source to the native disk /work/ckz_src (only changed files, deletions mirrored)
#   2. bind-mount /work/ckz_src at the original build path /Applications/buildAgent/work/c426877c688cbe05,
#      so __FILE__ strings and the debug compile directory are the original ones
#   3. make -j<cores> with clang dependency files: only sources whose file or included headers changed are
#      recompiled; a change of ckz_compile_one.sh (the flags) or of the include directories rebuilds all
# Products, always overwritten in place (no per-run directories):
#   /work/ckz_build/obj/<name>.o   objects (CFileUtil_gServe .cpp/.mm keep their original unique names)
#   /work/ckz_build/dep/<name>.d   dependency files
#   /work/ckz_build/log/<name>.log compiler output of the last compile
set -uo pipefail

# repository root = the parent of this toolchain directory (works from any checkout location)
TOOLCHAIN=$(cd "$(dirname "$0")" && pwd)
REPO=$(dirname "$TOOLCHAIN")
SOURCE_MIRROR=$REPO/CKZ_source
LOCAL=/work/ckz_src
ROOT=/Applications/buildAgent/work/c426877c688cbe05
SDK=/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk
SDK_SOURCE=/work/xcode421/packages/iPhoneSDK5_0/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk
# compiler used by ckz_compile_one.sh (b109: Xcode 4.2.1 clang-211.12 via maloader); hashed into the flag signature
LOADER=/work/maloader_src/maloader/ld-mac
CLANG=/work/xcode421/packages/clang/usr/bin/clang
# The two prebuilt blocks are assembled by the same clang's integrated assembler (-cc1as). It gives the
# same objects as llvm-mc of the rebuilt clang-211.10.1 used before (checked: identical SHA-256), so no
# LLVM has to be built from source. Never use the clang driver here: it would force thumbv7.
ASSEMBLER="$LOADER $CLANG -cc1as -filetype obj"
BUILD=/work/ckz_build
OUT=$BUILD/obj
DEPS=$BUILD/dep
LOGS=$BUILD/log
RULES=$BUILD/rules.mk
SIGNATURE=$BUILD/flags.sig
SWERVE_SOURCE=packages/swerve/code/cocoa/libs/swerve_core.s
# b115: prebuilt OpenSSL regenerated from the original binary (tools/openssl_blob.py); Thumb code
OPENSSL_SOURCE=packages/openssl/code/cocoa/libs/openssl_core.s
JOBS=$(nproc)

QUIET=0
FORCE=""
while [ $# -gt 0 ]; do
    case "$1" in
        -q) QUIET=1; shift ;;
        -B) FORCE="-B"; shift ;;
        *) break ;;
    esac
done
if [ $# -eq 0 ]; then
    echo "usage: build_ckz.sh [-q] [-B] (--all | <rel_source>...)"
    exit 2
fi

mkdir -p "$LOCAL" "$OUT" "$DEPS" "$LOGS" "$(dirname "$SDK")" /Applications/buildAgent/work

# b117: several writers may call this script at the same time (directly or via m3_check_unit.py);
# they share the rsync mirror, the bind mount and rules.mk, so whole runs are serialized with a lock.
# fd 9 stays open until the script exits, which releases the lock.
LOCK_FILE=$BUILD/.build.lock
exec 9>"$LOCK_FILE"
if ! flock -n 9; then
    echo "build_ckz.sh: waiting for another build (lock $LOCK_FILE)" >&2
    flock 9
fi

# 1. mirror the sources onto the native disk (mtimes preserved, so make sees real edits);
#    case aliases from the last run are kept, then refreshed (the Mac build was case-insensitive)
ALIASES=$BUILD/case_aliases.txt
touch "$ALIASES"
rsync -a --delete --exclude-from="$ALIASES" "$SOURCE_MIRROR/" "$LOCAL/"
python3 "$TOOLCHAIN/ckz_case_aliases.py" "$LOCAL" "$ALIASES"

# 2. original build path as a bind mount (a symlink would make the debug compile dir the real path)
if [ -L "$ROOT" ]; then
    rm "$ROOT"
fi
mkdir -p "$ROOT"
if ! mountpoint -q "$ROOT"; then
    mount --bind "$LOCAL" "$ROOT"
fi
[ -L "$SDK" ] || ln -s "$SDK_SOURCE" "$SDK"

# 3. include flags (Xcode header maps: every header directory) and the flags signature
INCLUDE_FLAGS=""
while read -r directory; do
    INCLUDE_FLAGS="$INCLUDE_FLAGS -I$ROOT/${directory#./}"
done < <(cd "$LOCAL" && find . -name '*.h' -printf '%h\n' | sort -u)
# b100: libogg/libvorbis include their public headers as <ogg/ogg.h> and "vorbis/codec.h"
INCLUDE_FLAGS="$INCLUDE_FLAGS -I$ROOT/packages/platformglu/src/logg/include -I$ROOT/packages/platformglu/src/lvorbis/include"
echo "$INCLUDE_FLAGS" > "$BUILD/include_flags.new"
signature=$( (cat "$TOOLCHAIN/ckz_compile_one.sh" "$BUILD/include_flags.new"; sha256sum "$CLANG") | sha256sum | cut -d' ' -f1)
mv -f "$BUILD/include_flags.new" "$BUILD/include_flags"
# rewrite the signature only when it changes, so its mtime marks the last flag change
if [ ! -f "$SIGNATURE" ] || [ "$(cat "$SIGNATURE")" != "$signature" ]; then
    echo "$signature" > "$SIGNATURE"
    echo "flags changed: every object will be rebuilt"
fi

# object name of a source (the two CFileUtil_gServe units keep their original N_OSO names)
object_base() {
    case "$1" in
        packages/gserve_1_6_0/src/gServe_platform/CFileUtil_gServe.cpp) echo CFileUtil_gServe-AC3402786EA00CAF ;;
        packages/gserve_1_6_0/src/gServe_platform/CFileUtil_gServe.mm) echo CFileUtil_gServe-AC3402786FA00CB0 ;;
        *) name=$(basename "$1"); echo "${name%.*}" ;;
    esac
}

# 4. make rules: one explicit rule per source, plus the Swerve library
mapfile -t ALL_SOURCES < <(cd "$LOCAL" && find . \( -name '*.cpp' -o -name '*.mm' -o -name '*.m' -o -name '*.c' \) | sed 's|^\./||' | sort)
declare -A BASE_OF
{
    echo "all:"
    for relative in "${ALL_SOURCES[@]}"; do
        base=$(object_base "$relative")
        BASE_OF[$relative]=$base
        echo "all: $OUT/$base.o"
        echo "$OUT/$base.o: $ROOT/$relative $SIGNATURE"
        # quoted: the checkout path may contain spaces
        printf "\t@bash '%s' %s %s\n" "$TOOLCHAIN/ckz_compile_one.sh" "$relative" "$base"
        echo "-include $DEPS/$base.d"
    done
    # Swerve: prebuilt library regenerated from the original binary; integrated assembler, not the clang driver
    echo "all: $OUT/swerve_core.o"
    echo "$OUT/swerve_core.o: $ROOT/$SWERVE_SOURCE"
    printf '\t@%s -triple armv7-apple-darwin10 %s -o %s.tmp > %s 2>&1 && mv -f %s.tmp %s && echo "ok   %s" || { rm -f %s.tmp; echo "FAIL %s (log %s)"; exit 1; }\n' \
        "$ASSEMBLER" "$ROOT/$SWERVE_SOURCE" "$OUT/swerve_core.o" "$LOGS/swerve_core.log" "$OUT/swerve_core.o" "$OUT/swerve_core.o" \
        "$SWERVE_SOURCE" "$OUT/swerve_core.o" "$SWERVE_SOURCE" "$LOGS/swerve_core.log"
    # OpenSSL: same, but Thumb code (the armv7 triple ignores .code 16 and emits ARM bl relocations)
    echo "all: $OUT/openssl_core.o"
    echo "$OUT/openssl_core.o: $ROOT/$OPENSSL_SOURCE"
    printf '\t@%s -triple thumbv7-apple-darwin10 %s -o %s.tmp > %s 2>&1 && mv -f %s.tmp %s && echo "ok   %s" || { rm -f %s.tmp; echo "FAIL %s (log %s)"; exit 1; }\n' \
        "$ASSEMBLER" "$ROOT/$OPENSSL_SOURCE" "$OUT/openssl_core.o" "$LOGS/openssl_core.log" "$OUT/openssl_core.o" "$OUT/openssl_core.o" \
        "$OPENSSL_SOURCE" "$OUT/openssl_core.o" "$OPENSSL_SOURCE" "$LOGS/openssl_core.log"
} > "$RULES"

TARGETS=()
if [ "$1" = "--all" ]; then
    TARGETS=(all)
else
    for relative in "$@"; do
        base=${BASE_OF[$relative]:-}
        if [ "$relative" = "$SWERVE_SOURCE" ]; then
            base=swerve_core
        fi
        if [ "$relative" = "$OPENSSL_SOURCE" ]; then
            base=openssl_core
        fi
        if [ -z "$base" ]; then
            echo "FAIL $relative (no such source)"
            exit 1
        fi
        TARGETS+=("$OUT/$base.o")
    done
fi

# 5. build
started=$(date +%s)
output=$(make -f "$RULES" -j"$JOBS" -k $FORCE "${TARGETS[@]}" 2>&1)
status=$?
compiled=$(echo "$output" | grep -c '^ok ')
failed=$(echo "$output" | grep -c '^FAIL ')
if [ "$QUIET" = 0 ]; then
    echo "$output" | grep -E '^(ok|FAIL) ' || true
else
    echo "$output" | grep -E '^FAIL ' || true
fi
# make's own errors (missing prerequisite etc.) that are not compile failures
echo "$output" | grep -E '^make: \*\*\* No rule' || true

# 6. remove products of deleted sources (only when the whole tree was requested)
removed=0
if [ "$1" = "--all" ]; then
    declare -A WANTED
    for base in "${BASE_OF[@]}"; do
        WANTED[$base]=1
    done
    WANTED[swerve_core]=1
    WANTED[openssl_core]=1
    for path in "$OUT"/*.o "$DEPS"/*.d "$LOGS"/*.log; do
        [ -e "$path" ] || continue
        name=$(basename "$path")
        name=${name%.*}
        if [ -z "${WANTED[$name]:-}" ]; then
            rm -f "$path"
            removed=$((removed + 1))
        fi
    done
    rm -f "$OUT"/*.o.tmp
fi

echo "built: compiled $compiled, failed $failed, removed stale $removed, $(( $(date +%s) - started ))s"
if [ "$status" != 0 ] || [ "$failed" != 0 ]; then
    exit 1
fi
exit 0
