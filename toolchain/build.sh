#!/bin/bash
# One-click build inside the build Linux (run as root after setup_distro.sh):
# compile every source (incremental) and link zombsniper into <repo>/_output.
# Usage: build.sh [-B]
#   -B  recompile every object even if it is up to date
set -euo pipefail

TOOLCHAIN=$(cd "$(dirname "$0")" && pwd)

FORCE=""
if [ "${1:-}" = "-B" ]; then
    FORCE="-B"
fi

if [ "$(id -u)" != 0 ]; then
    echo "build.sh: run as root (the build bind-mounts the original source path)" >&2
    exit 1
fi
AS=/work/xcode421/packages/DeveloperToolsCLI/usr/libexec/as/arm/as
if [ ! -x /work/maloader_src/maloader/ld-mac ] || [ ! -x /work/xcode421/packages/clang/usr/bin/clang ] || [ ! -x "$AS" ]; then
    echo "build.sh: toolchain missing, run setup first (setup.bat or toolchain/setup_distro.sh)" >&2
    exit 1
fi

echo "== compile"
bash "$TOOLCHAIN/build_ckz.sh" -q $FORCE --all
echo "== link"
bash "$TOOLCHAIN/link_ckz.sh"
