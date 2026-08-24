#!/usr/bin/env python3
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK_PATH = ROOT / "dependencies.lock.json"
SHA = re.compile(r"^[0-9a-f]{40}$")
DIGEST = re.compile(r"^ghcr\.io/[^@]+@sha256:[0-9a-f]{64}$")
CORE = re.compile(r"^CORES\+?=\s*(\S+)$")
PLATFORMS = ("tg5040", "tg5050")


def run(*args: str | Path) -> None:
    subprocess.run(args, check=True)


def validate_archive(path: Path) -> None:
    with zipfile.ZipFile(path) as archive:
        if archive.testzip() is not None:
            raise ValueError(f"corrupt zip entry in {path}")
        if not archive.namelist():
            raise ValueError(f"empty archive: {path}")
        for name in archive.namelist():
            if name.startswith(("/", "../")) or "/../" in name:
                raise ValueError(f"unsafe archive path: {name}")


def platform_cores(platform: str) -> list[str]:
    return [
        match.group(1)
        for line in (ROOT / "workspace" / platform / "cores" / "makefile")
        .read_text()
        .splitlines()
        if (match := CORE.match(line))
    ]


def patch_chain(platform: str, core: str) -> list[Path]:
    platform_patch = (
        ROOT / "workspace" / platform / "cores" / "patches" / f"{core}.patch"
    )
    common_dir = ROOT / "workspace" / "all" / "cores" / "patches" / core
    return ([platform_patch] if platform_patch.is_file() else []) + sorted(
        common_dir.glob("*.patch")
    )


def validate_patches(lock: dict) -> None:
    cores_by_platform = {platform: platform_cores(platform) for platform in PLATFORMS}
    locked_cores = lock["core_repositories"]
    unknown = set().union(*map(set, cores_by_platform.values())) - set(locked_cores)
    if unknown:
        raise SystemExit(
            f"platform cores missing from lock: {', '.join(sorted(unknown))}"
        )

    checked = 0
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        sources = root / "sources"
        sources.mkdir()
        for core, dependency in locked_cores.items():
            source = sources / core
            run("git", "init", "-q", source)
            run("git", "-C", source, "remote", "add", "origin", dependency["repo"])
            run(
                "git",
                "-C",
                source,
                "fetch",
                "-q",
                "--depth=1",
                "origin",
                dependency["sha"],
            )
            run("git", "-C", source, "checkout", "-q", "--detach", "FETCH_HEAD")

            for platform, platform_cores_ in cores_by_platform.items():
                if core not in platform_cores_:
                    continue
                worktree = root / f"{platform}-{core}"
                run(
                    "git",
                    "-C",
                    source,
                    "worktree",
                    "add",
                    "-q",
                    "--detach",
                    worktree,
                    "HEAD",
                )
                try:
                    for patch in patch_chain(platform, core):
                        run("git", "-C", worktree, "apply", "--check", "-p1", patch)
                        run("git", "-C", worktree, "apply", "-p1", patch)
                finally:
                    run("git", "-C", source, "worktree", "remove", "--force", worktree)
                checked += 1
    print(f"patch chains: {checked} passed")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--remotes", action="store_true")
    parser.add_argument("--assets", action="store_true")
    parser.add_argument("--patches", action="store_true")
    args = parser.parse_args()
    try:
        lock = json.loads(LOCK_PATH.read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"invalid dependency lock: {error}") from error
    if lock.get("schema_version") != 1:
        raise SystemExit("unsupported lock schema")
    for section in ("core_repositories", "build_repositories"):
        for name, dependency in lock[section].items():
            if not dependency["repo"].startswith("https://") or not SHA.fullmatch(
                dependency["sha"]
            ):
                raise SystemExit(f"invalid {section} entry: {name}")
    if set(lock["core_repositories"]) != set(
        "a2600 a5200 a7800 handy c64 c128 fake-08 fbneo fceumm gambatte gpsp "
        "libretro-cap32 libretro-uae mednafen_pce_fast mednafen_supafaust mednafen_vb "
        "mgba pcsx_rearmed pet picodrive plus4 pokemini race snes9x vic prboom bluemsx gearcoleco".split()
    ):
        raise SystemExit("core lock does not cover tg5040/tg5050 cores")
    for name, image in lock["toolchains"].items():
        if name not in PLATFORMS or not DIGEST.fullmatch(image):
            raise SystemExit(f"invalid toolchain lock: {name}")
    for name, asset in lock["release_assets"].items():
        if (
            not name.endswith(".pakz")
            or "/releases/download/" not in asset["url"]
            or not re.fullmatch(r"[0-9a-f]{64}", asset["sha256"])
        ):
            raise SystemExit(f"invalid release asset lock: {name}")
    if args.remotes:
        seen = set()
        for section in ("core_repositories", "build_repositories"):
            for dependency in lock[section].values():
                key = (dependency["repo"], dependency["sha"])
                if key in seen:
                    continue
                seen.add(key)
                with tempfile.TemporaryDirectory() as directory:
                    run("git", "init", "-q", directory)
                    run("git", "-C", directory, "remote", "add", "origin", key[0])
                    run(
                        "git",
                        "-C",
                        directory,
                        "fetch",
                        "-q",
                        "--depth=1",
                        "origin",
                        key[1],
                    )
    if args.assets:
        if shutil.which("curl") is None:
            raise SystemExit("curl is required to validate assets")
        with tempfile.TemporaryDirectory() as directory:
            for name, asset in lock["release_assets"].items():
                path = Path(directory, name)
                run(
                    "curl",
                    "--fail",
                    "--show-error",
                    "--location",
                    "--retry",
                    "3",
                    "--retry-all-errors",
                    "--output",
                    str(path),
                    asset["url"],
                )
                if hashlib.sha256(path.read_bytes()).hexdigest() != asset["sha256"]:
                    raise SystemExit(f"checksum mismatch: {name}")
                validate_archive(path)
    if args.patches:
        validate_patches(lock)


if __name__ == "__main__":
    main()
