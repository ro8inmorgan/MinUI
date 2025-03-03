# Toolchain Docker image

Now M1 Mac compatible!

Based on the [Trimui toolchain Docker image](https://git.crowdedwood.com/trimui-toolchain/) by neonloop. The toolchain blobs were created with this [Miyoo Mini Toolchain Buildroot](https://github.com/shauninman/miyoomini-toolchain-buildroot/) (or the [aarch64 version](https://github.com/shauninman/miyoomini-toolchain-buildroot-aarch64/)) and include the [GNU Toolchain for the A-profile Architecture 8.3-2019.03](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads/8-3-2019-03) (or the manually built [aarch64 version](https://github.com/shauninman/gcc-arm-8.3-aarch64)).

## Installation

With Docker installed and running, `make shell` builds the toolchain and drops into a shell inside the container. The container's `~/workspace` is bound to `./workspace` by default. The `CROSS_COMPILE` and `PATH` env vars have been updated with the toolchain location.

After building the first time, `make shell` will skip building and drop into the shell.

## Workflow

- On your host machine, clone repositories into `./workspace` and make changes as usual.
- In the container shell, find the repository in `~/workspace` and build as usual.

## Docker for Mac

Docker for Mac has a memory limit that can make the toolchain build fail. Follow [these instructions](https://docs.docker.com/docker-for-mac/) to increase the memory limit.
