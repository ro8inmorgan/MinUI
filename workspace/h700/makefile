###########################################################

ifeq (,$(PLATFORM))
PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
$(error please specify PLATFORM, eg. PLATFORM=rgb30 make)
endif

###########################################################

REQUIRES_EVTEST=other/evtest
REQUIRES_JSTEST=other/jstest
REQUIRES_UNZIP60=other/unzip60
REQUIRES_FILEMGR=other/NextCommander
REQUIRES_SDL2=other/sdl2
SDL2_COMMIT=d4a7d7503524cc469fe775242f3f925d4dd56c88
FILEMGR_PATCH=patches/NextCommander-h700.patch
FILEMGR_STAMP=.patched-h700-v2
SDL2_STAMP=.configured-nextui-h700-v3

all: readmes

early: $(REQUIRES_EVTEST) $(REQUIRES_JSTEST) $(REQUIRES_UNZIP60) $(REQUIRES_FILEMGR) $(REQUIRES_SDL2)
	mkdir -p other
	cd $(REQUIRES_EVTEST) && $(CROSS_COMPILE)gcc -o evtest evtest.c
	cd $(REQUIRES_JSTEST) && $(CROSS_COMPILE)gcc -o jstest jstest.c
	cd $(REQUIRES_UNZIP60) && make -f unix/Makefile.trimuismart unzip
	cd $(REQUIRES_SDL2) && ([ -f $(SDL2_STAMP) ] || ([ ! -f Makefile ] || make clean))
	cd $(REQUIRES_SDL2) && ( [ -f configure ] || ./autogen.sh ) && ./configure --host=$(CROSS_TRIPLE) --prefix=$(PREFIX_LOCAL) --enable-video-mali --enable-alsa --enable-alsa-shared --enable-video-opengles --disable-video-opengl --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm --disable-pulseaudio --disable-jack --disable-esd --disable-arts --disable-nas --disable-sndio --disable-dbus --disable-ibus --disable-fcitx --disable-hidapi --disable-libudev --disable-power --enable-filesystem --disable-sensor --disable-locale --enable-loadso --enable-shared --disable-static --disable-libsamplerate-shared && touch $(SDL2_STAMP)
	grep -q '^#define SDL_VIDEO_DRIVER_MALI 1' $(REQUIRES_SDL2)/include/SDL_config.h
	cd $(REQUIRES_SDL2) && make -j && make install
	mkdir -p $(REQUIRES_SDL2)/output
	cp -a $(PREFIX_LOCAL)/lib/libSDL2*.so* $(REQUIRES_SDL2)/output/
	cp -a $(PREFIX)/lib/libSDL2_image*.so* $(REQUIRES_SDL2)/output/ 2>/dev/null || true
	cp -a $(PREFIX)/lib/libSDL2_ttf*.so* $(REQUIRES_SDL2)/output/ 2>/dev/null || true
	cp -L $(PREFIX)/lib/libtinyalsa.so* $(REQUIRES_SDL2)/output/ 2>/dev/null || true
	# SDL2_image dlopens libpng12.so.0 at runtime; the H700 stock OS only has
	# a 32-bit copy under /mnt/vendor/lib, so bundle the toolchain's 64-bit one
	cp -L $(PREFIX)/lib/libpng12.so.0 $(REQUIRES_SDL2)/output/
	cd $(REQUIRES_FILEMGR) && ([ -f $(FILEMGR_STAMP) ] || (git checkout -- Makefile && git apply --unidiff-zero ../../$(FILEMGR_PATCH) && touch $(FILEMGR_STAMP)))
	cd $(REQUIRES_FILEMGR) && ([ -f output/.platform-h700 ] || make clean PLATFORM=h700)
	cd $(REQUIRES_FILEMGR) && make -j PLATFORM=h700 && touch output/.platform-h700
	cd boot && sh build.sh

clean:
	cd $(REQUIRES_FILEMGR) && make clean PLATFORM=h700
	cd $(REQUIRES_UNZIP60) && make -f unix/Makefile.trimuismart clean
	if [ -f $(REQUIRES_SDL2)/Makefile ]; then cd $(REQUIRES_SDL2) && make clean; fi
	rm -rf boot/output

###########################################################

$(REQUIRES_FILEMGR):
	git clone --depth 1 https://github.com/LoveRetro/NextCommander.git $(REQUIRES_FILEMGR)
	
$(REQUIRES_EVTEST):
	git clone --depth 1 https://github.com/freedesktop-unofficial-mirror/evtest.git $(REQUIRES_EVTEST)

$(REQUIRES_JSTEST):
	git clone --depth 1 https://github.com/datrh/joyutils.git $(REQUIRES_JSTEST)

$(REQUIRES_UNZIP60):
	git clone --depth 1 https://github.com/shauninman/unzip60.git $(REQUIRES_UNZIP60)

$(REQUIRES_SDL2):
	git clone https://github.com/JohnnyonFlame/SDL-malifbdev-rot.git $(REQUIRES_SDL2)
	cd $(REQUIRES_SDL2) && git checkout $(SDL2_COMMIT)

include ../all/readmes/makefile
