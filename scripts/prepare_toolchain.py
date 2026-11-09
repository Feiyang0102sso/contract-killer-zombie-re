"""Install the pinned LLVM tools for the Win32 MSBuild projects."""

import argparse
import hashlib
from pathlib import Path
import subprocess


LLVM_URL = "https://github.com/llvm/llvm-project/releases/download/llvmorg-21.1.8/LLVM-21.1.8-win64.exe?download=1"
LLVM_SHA256 = "7a5386c26497db1691f320121e5b113364dd0274b98e55f15f4dbc00c0450113"


def sha256(path: Path) -> str:
    """Hash the installer without loading it into memory."""
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def install_llvm(cache: Path) -> None:
    """Download, verify and install the compiler and linker in the local cache."""
    install_dir = cache / "LLVM"
    compiler = install_dir / "bin/clang-cl.exe"
    linker = install_dir / "bin/lld-link.exe"
    if compiler.is_file() and linker.is_file():
        return

    cache.mkdir(parents=True, exist_ok=True)
    installer = cache / "llvm.exe"
    if not installer.is_file() or sha256(installer) != LLVM_SHA256:
        temporary = cache / "llvm.exe.download"
        print("Downloading LLVM 21.1.8", flush=True)
        subprocess.run(
            ["curl.exe", "--fail", "--location", "--silent", "--show-error", "--retry", "2",
             LLVM_URL, "--output", str(temporary)],
            check=True,
        )
        if sha256(temporary) != LLVM_SHA256:
            raise ValueError("Could not verify the LLVM installer")
        temporary.replace(installer)

    print(f"Extracting LLVM to {install_dir}", flush=True)
    # NSIS reads the rest of the command line after /D= as the directory.
    subprocess.run(
        f'"{installer}" /S /D={install_dir}',
        executable=str(installer),
        check=True,
    )
    if not compiler.is_file() or not linker.is_file():
        raise FileNotFoundError(f"LLVM setup did not populate {install_dir}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, required=True)
    args = parser.parse_args()
    install_llvm(args.cache.resolve())


if __name__ == "__main__":
    main()
