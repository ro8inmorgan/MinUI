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

TARGET = minarch
PRODUCT= build/$(PLATFORM)/$(TARGET).elf
INCDIR = -I. -I./libretro-common/include/ -I../common/ -I../../$(PLATFORM)/platform/

# C and C++ translation units are listed separately. C files compile as C;
# C++ files compile as C++; everything links through g++ so the two can mix
# as modules are migrated to C++ incrementally.
CSOURCE =
CXXSOURCE = ../../$(PLATFORM)/platform/platform.cpp $(TARGET).cpp ../common/notification.cpp ../common/utils.cpp ../common/config.cpp ../common/scaler.cpp ../common/api.cpp \
         ma_audio.cpp ma_input.cpp ma_runframe.cpp ma_cheats.cpp ma_core.cpp \
         ma_rewind.cpp ma_saves.cpp ma_game.cpp ma_environment.cpp \
         ma_options.cpp ma_video.cpp ma_menu.cpp ma_frontend_opts.cpp ma_config.cpp

# RA support
ifneq (,$(filter $(PLATFORM),tg5040 tg5050 my355 desktop))
# RA source files (shared common/ helpers stay C; the two minarch-local RA units
# are C++). ra_integration sits on the rcheevos C callback boundary — kept
# mechanical, no throwing constructs on that path.
CXXSOURCE += ra_integration.cpp chd_reader.cpp ../common/ra_offline.cpp ../common/ra_sync.cpp ../common/ra_event_queue.cpp ../common/http.cpp ../common/ra_badges.cpp
endif

CC  = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
# Prefer real C++20; fall back to gnu++2a on older toolchains (e.g. the current
# gcc 8.3 tg5040 cross-compiler, which predates the -std=gnu++20 spelling).
CXXSTD := $(shell printf 'int main(){return 0;}' | $(CXX) -std=gnu++20 -x c++ - -o /dev/null >/dev/null 2>&1 && echo gnu++20 || echo gnu++2a)
CFLAGS  += $(OPT)
CFLAGS  += $(INCDIR) -DPLATFORM=\"$(PLATFORM)\"
# CXXFLAGS inherits the platform CFLAGS (SDL/GL/PREFIX include paths) before the
# C-only -std is appended, so C++ TUs get the same include/define environment.
CXXFLAGS += $(CFLAGS) -std=$(CXXSTD)
CFLAGS  += -std=gnu99
LDFLAGS	 += -lmsettings -lsamplerate

ifeq ($(PROFILE), 1)
CFLAGS  += -pg
LDFLAGS += -pg
endif

ifeq ($(GPERFTOOLS), 1)
CFLAGS  += -g
LDFLAGS += -lprofiler
endif

# Rewind support
LDFLAGS  += -llz4

# SaveRAM support
ifeq ($(PLATFORM), desktop)
ifeq ($(UNAME_S),Linux)
CFLAGS += `pkg-config --cflags libzip`
LDFLAGS += `pkg-config --libs libzip` -lz
else
LDFLAGS += -lzip -lz
endif # Linux
else # all handheld platforms
LDFLAGS	 +=  -Llibretro-common -lsrm -lzip -lz
CFLAGS   += -DHAS_SRM
LDFLAGS +=  -lasound
endif

# RA support: rcheevos and libchdr linking
ifneq (,$(filter $(PLATFORM),tg5040 tg5050 my355 desktop))
LDFLAGS += -lrcheevos -lchdr -lcrypto
CFLAGS += -DRC_CLIENT_SUPPORTS_HASH -DHAS_CHEEVOS -DHAS_CHDR
ifeq ($(PLATFORM), desktop)
# Desktop needs rpath for local shared library lookup
LDFLAGS += -Wl,-rpath,'$$ORIGIN'
endif
endif

# Sanitizers
ifeq ($(PLATFORM), desktop)
ifeq ($(TSAN), 1)
CFLAGS  += -fsanitize=thread
LDFLAGS += -ltsan
endif
ifeq ($(ASAN), 1)
CFLAGS  += -fsanitize=address -fno-common -fno-omit-frame-pointer
LDFLAGS += -lasan
endif
endif

AR=$(CROSS_COMPILE)ar
ARFLAGS=rc

BUILD_DATE!=date +%Y.%m.%d
BUILD_HASH!=cat ../../hash.txt
CFLAGS += -DBUILD_DATE=\"${BUILD_DATE}\" -DBUILD_HASH=\"${BUILD_HASH}\"

OBJ_DIR = build/$(PLATFORM)
OBJS = $(addprefix $(OBJ_DIR)/,$(notdir $(CSOURCE:.c=.o))) \
       $(addprefix $(OBJ_DIR)/,$(notdir $(CXXSOURCE:.cpp=.o)))
DEPS = $(OBJS:.o=.d)

# Locate sources across their directories so single pattern rules suffice.
vpath %.c   . ../common ../../$(PLATFORM)/platform
vpath %.cpp . ../common ../../$(PLATFORM)/platform

ifeq ($(PLATFORM), desktop)
all: clean libretro-common rcheevos libchdr $(PREFIX_LOCAL)/include/msettings.h $(PRODUCT)
else ifneq (,$(filter $(PLATFORM),tg5040 tg5050 my355))
# Platforms with RA support
all: clean libretro-common libsrm.a rcheevos libchdr $(PREFIX_LOCAL)/include/msettings.h copylibs $(PRODUCT)
endif

# Runtime shared libraries copied next to the binary on handheld platforms.
copylibs: | $(OBJ_DIR)
	cp -L $(PREFIX)/lib/libsamplerate.so.* build/$(PLATFORM)
	cp -L $(PREFIX)/lib/libcrypto.so.* build/$(PLATFORM)
	chmod a+r build/$(PLATFORM)/libcrypto.so.*
ifeq ($(PLATFORM), my355)
	cp -L $(PREFIX)/lib/libsqlite3.so build/$(PLATFORM)
	cp -L $(PREFIX)/lib/libffi.so.7 build/$(PLATFORM)
	cp -L $(PREFIX)/lib/libtinyalsa.so.2 build/$(PLATFORM)
endif
	cp -L $(PREFIX)/lib/libsamplerate.so.0 build/$(PLATFORM)
	cp -L $(PREFIX)/lib/libzip.so.5 build/$(PLATFORM)
	cp -L $(PREFIX)/lib/libbz2.so.1.0 build/$(PLATFORM)
	cp -L $(PREFIX)/lib/liblzma.so.5 build/$(PLATFORM)
	cp -L $(PREFIX)/lib/libzstd.so.1 build/$(PLATFORM)
	cp -L $(PREFIX)/lib/liblz4.so.1 build/$(PLATFORM)
	cp -L libchdr/build/$(PLATFORM)/libchdr.so.0 build/$(PLATFORM)

$(PRODUCT): $(OBJS)
	$(CXX) $(OBJS) -o $(PRODUCT) $(LDFLAGS) -lstdc++

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

libretro-common:
	git clone https://github.com/libretro/libretro-common

rcheevos: rcheevos/makefile $(PREFIX_LOCAL)/lib/librcheevos.a

libchdr: libchdr/CMakeLists.txt $(PREFIX_LOCAL)/lib/libchdr.so

rcheevos/makefile:
	rm -rf rcheevos
	mkdir -p rcheevos
	cp rcheevos.makefile rcheevos/makefile

$(PREFIX_LOCAL)/lib/librcheevos.a:
	cd rcheevos && make build install PLATFORM=$(PLATFORM)

libchdr/CMakeLists.txt:
	rm -rf libchdr
	git clone https://github.com/rtissera/libchdr
	cp libchdr.makefile libchdr/makefile

$(PREFIX_LOCAL)/lib/libchdr.so:
	cd libchdr && make build install PLATFORM=$(PLATFORM)
  
$(PREFIX_LOCAL)/include/msettings.h: ../../$(PLATFORM)/libmsettings/msettings.h
	cd ../../$(PLATFORM)/libmsettings && make

### libsrm stuff
OBJECTS = streams/rzip_stream.o streams/file_stream.o vfs/vfs_implementation.o file/file_path.o file/file_path_io.o compat/compat_strl.o time/rtime.o string/stdstring.o encodings/encoding_utf.o streams/trans_stream.o streams/trans_stream_pipe.o streams/trans_stream_zlib.o

$(OBJECTS) : 
	cd libretro-common && $(CC) $(CFLAGS) -DHAVE_ZLIB -c -o $@ $(subst .o,.c,$@) -I include/

libsrm.a: $(OBJECTS)
	cd libretro-common && $(AR) $(ARFLAGS) $@ $?

-include $(DEPS)

clean:
	rm -f $(PRODUCT)
	rm -f $(OBJS) $(DEPS)
	rm -f $(OBJECTS) $(LIBRARY)