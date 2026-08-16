###########################################################

ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
	$(error please specify PLATFORM, eg. PLATFORM=trimui make)
endif

ifeq (,$(CROSS_COMPILE))
	$(error missing CROSS_COMPILE for this toolchain)
endif

ifeq (,$(PREFIX))
	$(error missing PREFIX for this toolchain)
endif

###########################################################

include ../../$(PLATFORM)/platform/makefile.env
SDL?=SDL

###########################################################

TARGET=batmondb

.PHONY: build
.PHONY: clean

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
# Prefer real C++20; fall back to gnu++2a on older toolchains (e.g. the current
# gcc 8.3 tg5040 cross-compiler, which predates the -std=gnu++20 spelling).
CXXSTD := $(shell printf 'int main(){return 0;}' | $(CXX) -std=gnu++20 -x c++ - -o /dev/null >/dev/null 2>&1 && echo gnu++20 || echo gnu++2a)

SYSROOT     := $(shell $(CC) --print-sysroot)

INCDIR = -I. -I../common/ -I../../$(PLATFORM)/platform/
# The library itself is C++20 and pulls in only common/utils. (It used to also
# compile the platform TU, but that object was never linked into the .so.)
CXXSOURCE = $(TARGET).cpp ../common/utils.cpp

CFLAGS  += $(OPT)
CFLAGS  += $(INCDIR) -DPLATFORM=\"$(PLATFORM)\" -std=gnu99
# -Wno-write-strings: the exported functions call common C helpers that take
# char* (exists/touch); passing string literals is benign but would error under
# C++ -Werror.
CXXFLAGS = $(OPT) $(INCDIR) -DPLATFORM=\"$(PLATFORM)\" -std=$(CXXSTD) -Wno-write-strings
LDFLAGS += -s -lsqlite3

PRODUCT= build/$(PLATFORM)/lib$(TARGET).so

build:
	mkdir -p build/$(PLATFORM)
	$(CXX) -c -Werror -fpic $(CXXSOURCE) $(CXXFLAGS)
	$(CXX) -shared -o $(PRODUCT) "$(TARGET).o" "utils.o" $(LDFLAGS) -lstdc++
	mkdir -p "$(PREFIX_LOCAL)/include"
	mkdir -p "$(PREFIX_LOCAL)/lib"
	cp "$(TARGET).h" "$(PREFIX_LOCAL)/include"
	cp $(PRODUCT) "$(PREFIX_LOCAL)/lib"

clean:
	rm -f *.o
	rm -f $(PRODUCT)
	rm -f $(PREFIX_LOCAL)/include/$(TARGET).h
	rm -f $(PREFIX_LOCAL)/lib/lib$(TARGET).so