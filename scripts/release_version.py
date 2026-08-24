#!/usr/bin/env python3
import argparse
import re
import subprocess

VERSION = re.compile(
    r"^v?(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(?:-[0-9A-Za-z.-]+)?$"
)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("bump", choices=("major", "minor", "patch"))
    args = parser.parse_args()
    tags = subprocess.check_output(("git", "tag", "--list"), text=True).splitlines()
    versions = []
    for tag in tags:
        match = VERSION.fullmatch(tag)
        if match:
            versions.append(tuple(map(int, match.groups()[:3])))
    major, minor, patch = max(versions, default=(0, 0, 0))
    if args.bump == "major":
        major, minor, patch = major + 1, 0, 0
    elif args.bump == "minor":
        minor, patch = minor + 1, 0
    else:
        patch += 1
    print(f"v{major}.{minor}.{patch}")


if __name__ == "__main__":
    main()
