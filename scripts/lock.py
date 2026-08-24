#!/usr/bin/env python3
"""Read the dependency lock without adding a YAML/JSON CLI dependency."""

import json
import sys
from pathlib import Path

LOCK = Path(__file__).resolve().parents[1] / "dependencies.lock.json"


def main() -> None:
    if len(sys.argv) not in (3, 4):
        raise SystemExit("usage: lock.py <core|build|toolchain> <name> [repo|sha]")
    kind, name = sys.argv[1:3]
    try:
        lock = json.loads(LOCK.read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"invalid dependency lock: {error}") from error
    if kind == "toolchain":
        print(lock["toolchains"][name])
        return
    section = {"core": "core_repositories", "build": "build_repositories"}[kind]
    field = sys.argv[3] if len(sys.argv) == 4 else "sha"
    print(lock[section][name][field])


if __name__ == "__main__":
    main()
