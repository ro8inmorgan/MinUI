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

###########################################################

include ../../$(PLATFORM)/platform/makefile.env

###########################################################

TARGET = nextval
INCDIR = -I. -I../common/ -I../../$(PLATFORM)/platform/
# Common utils/config are now C++20 too; single g++ compile+link, no C sources.
CXXSOURCE = $(TARGET).cpp ../common/utils.cpp ../common/config.cpp

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
# Prefer real C++20; fall back to gnu++2a on older toolchains (e.g. the current
# gcc 8.3 tg5040 cross-compiler, which predates the -std=gnu++20 spelling).
CXXSTD := $(shell printf 'int main(){return 0;}' | $(CXX) -std=gnu++20 -x c++ - -o /dev/null >/dev/null 2>&1 && echo gnu++20 || echo gnu++2a)
CFLAGS  += $(OPT)
CFLAGS  += $(INCDIR) -DPLATFORM=\"$(PLATFORM)\"
CXXFLAGS += $(CFLAGS) -std=$(CXXSTD)

PRODUCT= build/$(PLATFORM)/$(TARGET).elf

all:
	mkdir -p build/$(PLATFORM)
	$(CXX) $(CXXSOURCE) -o $(PRODUCT) $(CXXFLAGS) $(LDFLAGS) -lstdc++
clean:
	rm -f $(PRODUCT)
