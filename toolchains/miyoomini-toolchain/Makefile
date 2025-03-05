.PHONY: shell
.PHONY: clean
	
TOOLCHAIN_NAME=miyoomini-toolchain
WORKSPACE_DIR := $(shell pwd)/workspace

.build: Dockerfile
	mkdir -p ./workspace
	docker build -t $(TOOLCHAIN_NAME) .
	touch .build

shell: .build
	docker run -it --rm -v "$(WORKSPACE_DIR)":/root/workspace $(TOOLCHAIN_NAME) /bin/bash

clean:
	docker rmi $(TOOLCHAIN_NAME)
	rm -f .build
