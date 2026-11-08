#!/bin/bash
# b107: link the whole game into the armv7 Mach-O executable zombsniper (first link, not byte-exact).
#
# Needs an ld64 that runs on Linux. Xcode 4.2.1's ld (ld64-127.2) is an x86_64 Mach-O binary; b107 runs it
# through maloader (patched, see patches/maloader.patch). setup_distro.sh prepares both; the defaults below
# are where it puts them. Override with LD64 (the ld binary) and LD64_LOADER (maloader's ld-mac, may be empty
# for a native Linux ld64).
# Output: /work/ckz_build/link/zombsniper, copied to $CKZ_OUTPUT_DIR (default: <repo>/_output).
#
# The command line is the one clang-211's driver produces for
#   clang -arch armv7 -miphoneos-version-min=4.0 -isysroot $SDK -dead_strip ... -lstdc++
# (checked with -###), plus the frameworks/dylibs of the original LC_LOAD_DYLIB /
# LC_LOAD_WEAK_DYLIB commands, in the original order.
#
# Inputs:
#   /work/ckz_build/obj/*.o            game objects + the prebuilt blocks swerve_core.o and (b115)
#                                      openssl_core.o, all made by build_ckz.sh;
#                                      except lookup.o (libvorbis lookup.c is only #included by lsp.c)
#   (b107 linked a clang rebuild of OpenSSL 1.0.0d from toolchain/build_openssl.sh; b115 replaced it
#    with the byte-verified block of the original prebuilt OpenSSL)
# -dead_strip is required: libvorbis block.c / vorbisfile.c reference vorbis_lpc_* and
# _analysis_output_always only from functions the original linker stripped as well.
set -euo pipefail

TOOLCHAIN=$(cd "$(dirname "$0")" && pwd)
REPO=$(dirname "$TOOLCHAIN")
# ld lives next to ../lib/libLTO.dylib in this merged directory (setup_distro.sh, see the b107 note there)
LD64=${LD64:-/work/ckz_build/ld64_root/usr/bin/ld}
LD64_LOADER=${LD64_LOADER-/work/maloader_src/maloader/ld-mac}
SDK=/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk
OBJECTS=/work/ckz_build/obj
OUT_DIR=/work/ckz_build/link
OUTPUT=$OUT_DIR/zombsniper
EXCLUDED_OBJECTS="lookup.o"
# b109: original object order (N_OSO order of the STABS debug map)
LINK_ORDER=$TOOLCHAIN/link_order.txt
# Final products are copied here after every successful link (user rule, 2026-11-08)
FINAL_DIR=${CKZ_OUTPUT_DIR:-$REPO/_output}

# Original dynamic libraries, in LC_LOAD_DYLIB order (W = weak)
LIBRARIES=(
    "framework MessageUI"
    "weak_framework AVFoundation"
    "framework AudioToolbox"
    "framework StoreKit"
    "framework AddressBook"
    "weak_framework CFNetwork"
    "weak_framework CoreGraphics"
    "weak_framework CoreLocation"
    "framework Foundation"
    "framework MobileCoreServices"
    "framework QuartzCore"
    "framework SystemConfiguration"
    "weak_framework UIKit"
    "library sqlite3"
    "framework CoreTelephony"
    "framework OpenGLES"
    "framework CoreFoundation"
    "weak_framework MediaPlayer"
    "framework OpenAL"
    "framework Security"
    "framework AddressBookUI"
    "library z.1"
    "weak_framework GameKit"
    "weak_framework EventKit"
    "weak_framework EventKitUI"
    "weak_framework iAd"
    "weak_library System"
    "weak_framework CoreMotion"
    "weak_framework MapKit"
)

mkdir -p "$OUT_DIR"
ARGUMENTS=(-dynamic -arch armv7 -dead_strip -iphoneos_version_min 4.0.0 -syslibroot "$SDK" -o "$OUTPUT" -lcrt1.3.1.o)

# game objects: first in the original link order (b109, N_OSO order of the debug map), then the rest
# (objects the original only had inside prebuilt libraries, our stubs, ...) in name order
declare -A ADDED
OBJECT_COUNT=0
ORDERED_COUNT=0
add_object() {
    local name=$1
    local excluded
    for excluded in $EXCLUDED_OBJECTS; do
        if [ "$name" = "$excluded" ]; then
            return
        fi
    done
    if [ -n "${ADDED[$name]:-}" ] || [ ! -f "$OBJECTS/$name" ]; then
        return
    fi
    ADDED[$name]=1
    ARGUMENTS+=("$OBJECTS/$name")
    OBJECT_COUNT=$((OBJECT_COUNT + 1))
}
while read -r name; do
    case "$name" in
        ""|"#"*) continue ;;
    esac
    add_object "$name"
done < "$LINK_ORDER"
ORDERED_COUNT=$OBJECT_COUNT
for path in "$OBJECTS"/*.o; do
    add_object "$(basename "$path")"
done
echo "objects in original order: $ORDERED_COUNT, appended: $((OBJECT_COUNT - ORDERED_COUNT))"


# frameworks and dylibs
for entry in "${LIBRARIES[@]}"; do
    kind=${entry%% *}
    name=${entry#* }
    case "$kind" in
        framework)      ARGUMENTS+=(-framework "$name") ;;
        weak_framework) ARGUMENTS+=(-weak_framework "$name") ;;
        library)        ARGUMENTS+=("-l$name") ;;
        weak_library)   ARGUMENTS+=("-weak-l$name") ;;
    esac
done
# the driver's default libraries (libSystem, C++ runtime, libgcc_s), and libobjc used by the ObjC code.
# libSystem is already listed above as weak (original LC_LOAD_WEAK_DYLIB #27); ld ignores the repeat.
ARGUMENTS+=(-lstdc++ -lSystem -lgcc_s.1 -lobjc)

echo "link: $OBJECT_COUNT objects -> $OUTPUT"
printf '%s\n' "${ARGUMENTS[@]}" > "$OUT_DIR/link.args"
STATUS=0
$LD64_LOADER "$LD64" "${ARGUMENTS[@]}" > "$OUT_DIR/link.log" 2>&1 || STATUS=$?
echo "ld64 exit $STATUS (log $OUT_DIR/link.log)"
if [ $STATUS -eq 0 ]; then
    mkdir -p "$FINAL_DIR"
    cp -f "$OUTPUT" "$OUT_DIR/link.args" "$OUT_DIR/link.log" "$FINAL_DIR/"
    (cd "$FINAL_DIR" && sha256sum zombsniper > zombsniper.sha256)
    echo "copied to $FINAL_DIR: $(cat "$FINAL_DIR/zombsniper.sha256")"
fi
exit $STATUS
