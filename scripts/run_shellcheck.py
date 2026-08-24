#!/usr/bin/env python3
import os
import re
import subprocess
import sys
from pathlib import Path

SHELLS = {"bash", "sh"}
SHEBANG = re.compile(r"^#!\s*(.*)$")


def shell_for(path: Path) -> str | None:
    try:
        first = path.read_text(errors="replace").splitlines()[0]
    except (OSError, IndexError):
        return None
    match = SHEBANG.match(first)
    if not match:
        return None
    words = match.group(1).split()
    if not words:
        return None
    if os.path.basename(words[0]) == "env":
        words = words[1:]
        while words and words[0].startswith("-"):
            words = words[1:]
    if not words:
        return None
    shell = os.path.basename(words[0])
    return shell if shell in SHELLS else None


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    tracked = subprocess.check_output(("git", "ls-files", "-z"), cwd=root)
    groups: dict[str, list[str]] = {shell: [] for shell in SHELLS}
    for raw_path in tracked.split(b"\0"):
        if not raw_path:
            continue
        relative = raw_path.decode()
        path = root / relative
        shell = shell_for(path)
        if shell is None and not relative.endswith(".sh"):
            continue
        groups[shell or "sh"].append(relative)

    status = 0
    for shell in ("bash", "sh"):
        if not groups[shell]:
            continue
        result = subprocess.run(
            ("shellcheck", "--severity=warning", f"--shell={shell}", *groups[shell]),
            cwd=root,
        )
        status = max(status, result.returncode)
    return status


if __name__ == "__main__":
    sys.exit(main())
