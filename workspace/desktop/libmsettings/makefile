ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif
ifeq (,$(PREFIX))
$(error missing PREFIX for this toolchain)
endif

TARGET=msettings

.PHONY: build
.PHONY: clean

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
CFLAGS = -I../../all/common
# msettings is C++ but uses no C++ runtime; -fno-exceptions/-fno-rtti keeps the
# object free of libstdc++ so libmsettings.so still links with plain gcc.
# -Wno-write-strings: msettings passes string literals to char* helpers
# (touch/putInt); benign, tolerated elsewhere in the tree.
CXXFLAGS = $(CFLAGS) -std=gnu++2a -fno-exceptions -fno-rtti -Wno-write-strings

ifeq ($(PROFILE), 1)
CFLAGS  += -pg
LDFLAGS += -pg
endif

#LDFLAGS = -ldl -lrt -s
LDFLAGS = -ldl

OPTM=-Ofast

build:
	$(CXX) -c -Werror -fpic "$(TARGET).cpp" $(CXXFLAGS) -Wl,--no-as-needed $(LDFLAGS)
	$(CC) -shared -o "lib$(TARGET).so" "$(TARGET).o" $(LDFLAGS)
	mkdir -p "$(PREFIX_LOCAL)/include"
	mkdir -p "$(PREFIX_LOCAL)/lib"
	cp "$(TARGET).h" "$(PREFIX_LOCAL)/include"
	cp "lib$(TARGET).so" "$(PREFIX_LOCAL)/lib"

clean:
	rm -f *.o
	rm -f "lib$(TARGET).so"
	rm -f $(PREFIX_LOCAL)/include/$(TARGET).h
	rm -f $(PREFIX_LOCAL)/lib/lib$(TARGET).so
