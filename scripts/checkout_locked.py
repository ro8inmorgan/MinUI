#!/usr/bin/env python3
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def output(*args: str) -> str:
    return subprocess.check_output(args, text=True, stderr=subprocess.DEVNULL).strip()


def matches(path: Path, dependency: dict[str, str]) -> bool:
    try:
        return (
            output("git", "-C", str(path), "config", "--get", "remote.origin.url")
            == dependency["repo"]
            and output("git", "-C", str(path), "rev-parse", "HEAD") == dependency["sha"]
        )
    except subprocess.CalledProcessError:
        return False


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: checkout_locked.py <core|build> NAME DESTINATION")
    kind, name, destination = sys.argv[1:]
    try:
        lock = json.loads((ROOT / "dependencies.lock.json").read_text())
        dependency = lock[
            {"core": "core_repositories", "build": "build_repositories"}[kind]
        ][name]
    except (KeyError, OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"invalid locked dependency {kind}/{name}: {error}") from error

    path = Path(destination)
    if matches(path, dependency):
        return
    try:
        shutil.rmtree(path, ignore_errors=True)
        path.mkdir(parents=True)
    except OSError as error:
        raise SystemExit(f"cannot prepare checkout {path}: {error}") from error
    subprocess.run(("git", "-C", str(path), "init", "-q"), check=True)
    subprocess.run(
        ("git", "-C", str(path), "remote", "add", "origin", dependency["repo"]),
        check=True,
    )
    subprocess.run(
        (
            "git",
            "-C",
            str(path),
            "fetch",
            "-q",
            "--depth=1",
            "origin",
            dependency["sha"],
        ),
        check=True,
    )
    subprocess.run(
        ("git", "-C", str(path), "checkout", "-q", "--detach", "FETCH_HEAD"), check=True
    )
    if not matches(path, dependency):
        raise SystemExit(f"locked checkout did not match {kind}/{name}")


if __name__ == "__main__":
    main()
