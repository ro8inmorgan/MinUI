#!/bin/sh

cd "$(dirname "$0")"

# (Re)install the hooks that actually enforce the budget. Opening the pak once
# is what arms it -- there is no other moment a Tools pak gets to run.
mkdir -p "$HOOKS_PATH/pre-launch.d" "$HOOKS_PATH/post-launch.d"
cp -f ./hooks/parental-gate.sync.sh "$HOOKS_PATH/pre-launch.d/"
cp -f ./hooks/parental-stop.sh "$HOOKS_PATH/post-launch.d/"
chmod +x "$HOOKS_PATH/pre-launch.d/parental-gate.sync.sh" "$HOOKS_PATH/post-launch.d/parental-stop.sh"

./parental.elf # &> ./log.txt
