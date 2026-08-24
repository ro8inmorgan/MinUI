#!/usr/bin/env python3
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(*args: str | Path) -> str:
    return subprocess.check_output(args, text=True).strip()


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    script = root / "scripts" / "checkout_locked.py"
    script.parent.mkdir()
    shutil.copy2(ROOT / "scripts/checkout_locked.py", script)
    source = root / "source"
    destination = root / "destination"
    run("git", "init", "-q", source)
    run("git", "-C", source, "config", "user.email", "test@example.invalid")
    run("git", "-C", source, "config", "user.name", "test")
    (source / "source.txt").write_text("locked\n")
    run("git", "-C", source, "add", "source.txt")
    run("git", "-C", source, "commit", "-qm", "locked")
    locked_sha = run("git", "-C", source, "rev-parse", "HEAD")
    (source / "source.txt").write_text("stale\n")
    run("git", "-C", source, "commit", "-am", "stale", "-q")
    stale_sha = run("git", "-C", source, "rev-parse", "HEAD")
    (root / "dependencies.lock.json").write_text(
        json.dumps(
            {
                "build_repositories": {
                    "fixture": {"repo": str(source), "sha": locked_sha}
                }
            }
        )
    )

    def checkout() -> None:
        run(sys.executable, script, "build", "fixture", destination)

    checkout()
    run("git", "-C", destination, "fetch", "-q", "origin", stale_sha)
    run("git", "-C", destination, "checkout", "-q", "--detach", stale_sha)
    checkout()
    assert run("git", "-C", destination, "rev-parse", "HEAD") == locked_sha

    run("git", "-C", destination, "remote", "set-url", "origin", "not-the-lock")
    checkout()
    assert run("git", "-C", destination, "config", "--get", "remote.origin.url") == str(
        source
    )
    assert run("git", "-C", destination, "rev-parse", "HEAD") == locked_sha

print("stale locked checkout: passed")
