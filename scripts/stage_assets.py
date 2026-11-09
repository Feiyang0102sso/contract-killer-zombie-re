"""Stage the standalone Swerve ARM image shipped with the PC port."""

import argparse
from pathlib import Path


IMAGE_PATH = (
    Path(__file__).resolve().parents[1]
    / "CKZ_source/pc_port/swerve_compat/swerve-arm.bin"
)


def stage_swerve(output: Path) -> None:
    """Place the bundled Swerve image next to the executable."""
    image = IMAGE_PATH.read_bytes()
    if not output.is_file() or output.read_bytes() != image:
        output.parent.mkdir(parents=True, exist_ok=True)
        temporary = output.with_name(f"{output.name}.tmp")
        temporary.write_bytes(image)
        temporary.replace(output)
    print(f"Swerve ARM image ready: {output}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    stage_swerve(args.output)


if __name__ == "__main__":
    main()
