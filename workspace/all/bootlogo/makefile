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

TARGET = bootlogo
INCDIR = -I. -I../common/ -I../../$(PLATFORM)/platform/
SOURCE = $(TARGET).c ../common/utils.c ../common/api.c ../common/config.c ../common/scaler.c ../../$(PLATFORM)/platform/platform.c

CC = $(CROSS_COMPILE)gcc
CFLAGS  += $(OPT)
CFLAGS  += $(INCDIR) -DPLATFORM=\"$(PLATFORM)\" -std=gnu99
ifeq ($(PLATFORM),h700)
# H700 stores bootlogo.bmp on partition 2 and groups presets by panel resolution.
CFLAGS += -DBOOTLOGO_PARTITION=\"/dev/mmcblk0p2\" -DBOOTLOGO_RESOLUTION_DIRS=1
# Rotate RG28XX previews to match its portrait-mounted panel at boot.
CFLAGS += -DBOOTLOGO_PREVIEW_ROTATE_CW=needs_portrait_sdl
endif
LDFLAGS	 += -lmsettings

PRODUCT= build/$(PLATFORM)/$(TARGET).elf

all: $(PREFIX_LOCAL)/include/msettings.h
	mkdir -p build/$(PLATFORM)
	$(CC) $(SOURCE) -o $(PRODUCT) $(CFLAGS) $(LDFLAGS)
clean:
	rm -f $(PRODUCT)

$(PREFIX_LOCAL)/include/msettings.h:
	cd ../../$(PLATFORM)/libmsettings && make
