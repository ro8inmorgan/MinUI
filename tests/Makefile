.PHONY: all clean

SANITIZER ?= address,undefined
export SANITIZER

all:
	./run.sh

clean:
	rm -rf "$${BUILD_DIR:-/tmp/nextui-tests}"
