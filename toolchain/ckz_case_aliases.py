"""Create case aliases in the native source copy (run by build_ckz.sh inside ckre).

The original build ran on a case-insensitive Mac file system, and so does the Windows mirror, so
`#include "DGCamera.h"` finds src/dh3d/dgCamera.h there. The native copy on ext4 is case-sensitive:
for every quoted include whose file name exists only with a different case, a symlink with the
included spelling is placed next to the real file (the same path the Mac build saw).

Usage: python3 ckz_case_aliases.py <native source root> <alias list output>
The alias list (relative paths) is used by build_ckz.sh to keep rsync --delete from removing them.
"""
import re
import sys
from pathlib import Path

INCLUDE_PATTERN = re.compile(r'^\s*#\s*(?:include|import)\s*"([^"]+)"', re.MULTILINE)
SOURCE_SUFFIXES = (".c", ".cpp", ".m", ".mm", ".h", ".hpp", ".pch")


def main():
    root = Path(sys.argv[1])
    alias_list = Path(sys.argv[2])
    by_lower = {}
    exact = set()
    files = []
    for path in root.rglob("*"):
        if path.is_file() and not path.is_symlink():
            files.append(path)
            exact.add(path.name)
            by_lower.setdefault(path.name.lower(), []).append(path)

    aliases = []
    for path in files:
        if path.suffix not in SOURCE_SUFFIXES:
            continue
        for included in INCLUDE_PATTERN.findall(path.read_text(encoding="latin-1")):
            name = Path(included).name
            if name in exact or name.lower() not in by_lower:
                continue
            for target in by_lower[name.lower()]:
                alias = target.with_name(name)
                if not alias.exists():
                    alias.symlink_to(target.name)
                    print("case alias:", alias.relative_to(root), "->", target.name)
                aliases.append(alias.relative_to(root).as_posix())

    alias_list.write_text("".join(sorted(set(a + "\n" for a in aliases))), encoding="utf-8")


if __name__ == "__main__":
    main()
