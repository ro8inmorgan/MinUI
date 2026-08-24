#!/usr/bin/env python3
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def validate_archive(path: Path) -> None:
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None or not archive.namelist():
            raise ValueError(f"invalid archive: {path}")
        if any(
            name.startswith(("/", "../")) or "/../" in name
            for name in archive.namelist()
        ):
            raise ValueError(f"unsafe archive: {path}")


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: fetch_release_assets.py DESTINATION")
    destination = Path(sys.argv[1])
    destination.mkdir(parents=True, exist_ok=True)
    try:
        lock = json.loads((ROOT / "dependencies.lock.json").read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"invalid dependency lock: {error}") from error
    for name, asset in lock["release_assets"].items():
        target = destination / name
        with tempfile.NamedTemporaryFile(dir=destination, delete=False) as temporary:
            temporary_path = Path(temporary.name)
        try:
            subprocess.run(
                (
                    "curl",
                    "--fail",
                    "--show-error",
                    "--location",
                    "--retry",
                    "3",
                    "--retry-all-errors",
                    "--output",
                    str(temporary_path),
                    asset["url"],
                ),
                check=True,
            )
            if (
                hashlib.sha256(temporary_path.read_bytes()).hexdigest()
                != asset["sha256"]
            ):
                raise SystemExit(f"checksum mismatch: {name}")
            validate_archive(temporary_path)
            os.replace(temporary_path, target)
        finally:
            temporary_path.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
