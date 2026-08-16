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
SDL?=SDL

###########################################################

TARGET = settings
INCDIR = -I. -I../common/ -I../../$(PLATFORM)/platform/
SOURCE = -c ../../$(PLATFORM)/platform/platform.cpp
CXXSOURCE = ../common/palette.cpp ../common/utils.cpp ../common/config.cpp ../common/scaler.cpp ../common/api.cpp ../common/ra_offline.cpp ../common/ra_sync.cpp ../common/ra_event_queue.cpp ../common/http.cpp ../common/ra_auth.cpp $(TARGET).cpp menu.cpp colorpickermenu.cpp palettemenu.cpp fnbuttonmenu.cpp wifimenu.cpp btmenu.cpp keyboardprompt.cpp
CXXSOURCE += build/$(PLATFORM)/platform.o

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
# Prefer real C++20; fall back to gnu++2a on older toolchains (e.g. the current
# gcc 8.3 tg5040 cross-compiler, which predates the -std=gnu++20 spelling).
CXXSTD := $(shell printf 'int main(){return 0;}' | $(CXX) -std=gnu++20 -x c++ - -o /dev/null >/dev/null 2>&1 && echo gnu++20 || echo gnu++2a)
CFLAGS += $(OPT)
CFLAGS += $(INCDIR) -DPLATFORM=\"$(PLATFORM)\"
CXXFLAGS += $(CFLAGS) -std=$(CXXSTD)
CFLAGS += -fpermissive
LDFLAGS +=  -lmsettings -lcrypto
ifeq ($(PLATFORM), tg5040)
# drop as soon as we use btagent on all platforms
CXXFLAGS += -DHAS_BTAGENT
CXXSOURCE += btagent.cpp
CXXFLAGS += $$(pkg-config --cflags --libs gio-2.0 glib-2.0)
endif
ifeq ($(PLATFORM), tg5050)
# drop as soon as we use btagent on all platforms
CXXFLAGS += -DHAS_BTAGENT
CXXSOURCE += btagent.cpp
CXXFLAGS += $$(pkg-config --cflags --libs gio-2.0 glib-2.0)
endif

PRODUCT= build/$(PLATFORM)/$(TARGET).elf

all: $(PREFIX_LOCAL)/include/msettings.h
	mkdir -p build/$(PLATFORM)
	$(CXX) $(SOURCE) $(CXXFLAGS) $(LDFLAGS)
	mv platform.o build/$(PLATFORM)
	$(CXX) $(CXXSOURCE) -o $(PRODUCT) $(CXXFLAGS) $(LDFLAGS) -lstdc++
clean:
	rm -f $(PRODUCT)

$(PREFIX_LOCAL)/include/msettings.h: ../../$(PLATFORM)/libmsettings/msettings.h
	cd ../../$(PLATFORM)/libmsettings && make
