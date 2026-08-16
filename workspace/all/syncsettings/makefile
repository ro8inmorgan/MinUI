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

TARGET = syncsettings
# Single C++20 translation unit, no shared common/ deps; link through g++.
SOURCE = $(TARGET).cpp

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
# Prefer real C++20; fall back to gnu++2a on older toolchains (e.g. the current
# gcc 8.3 tg5040 cross-compiler, which predates the -std=gnu++20 spelling).
CXXSTD := $(shell printf 'int main(){return 0;}' | $(CXX) -std=gnu++20 -x c++ - -o /dev/null >/dev/null 2>&1 && echo gnu++20 || echo gnu++2a)
CXXFLAGS = $(OPT) -I$(PREFIX_LOCAL)/include -DPLATFORM=\"$(PLATFORM)\" -std=$(CXXSTD)
LDFLAGS	 = -Os -L$(PREFIX_LOCAL)/lib -lmsettings -lrt -ldl -Wl,--gc-sections -s

PRODUCT= build/$(PLATFORM)/$(TARGET).elf

all: $(PREFIX_LOCAL)/include/msettings.h
	mkdir -p build/$(PLATFORM)
	$(CXX) $(SOURCE) -o $(PRODUCT) $(CXXFLAGS) $(LDFLAGS) -lstdc++
clean:
	rm -f $(PRODUCT)

$(PREFIX_LOCAL)/include/msettings.h: ../../$(PLATFORM)/libmsettings/msettings.h
	cd ../../$(PLATFORM)/libmsettings && make
