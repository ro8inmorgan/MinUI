# pi-lens-ignore: SC1073, SC1065, SC1064, SC1072
ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

TARGET = keymon
PRODUCT = $(TARGET).elf

CC = $(CROSS_COMPILE)gcc
CFLAGS	= -Os -lmsettings -lpthread -lrt -ldl -Wl,--gc-sections -s
CFLAGS  += -I. -I../../all/common -I../platform/ -I$(PREFIX_LOCAL)/include -L$(PREFIX_LOCAL)/lib -DPLATFORM=\"$(UNION_PLATFORM)\"

all: $(PREFIX)/include/msettings.h
	$(CC) $(TARGET).c -o $(PRODUCT) $(CFLAGS)
clean:
	rm -rf $(PRODUCT)

$(PREFIX)/include/msettings.h:
	cd ../libmsettings && make
