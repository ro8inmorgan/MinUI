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

all: readmes

early: $(REQUIRES_EVTEST) $(REQUIRES_JSTEST) $(REQUIRES_UNZIP60) $(REQUIRES_FILEMGR) $(REQUIRES_SDL2)
	mkdir -p other
	cd $(REQUIRES_FILEMGR) && make -j PLATFORM=tg5040
	cd $(REQUIRES_EVTEST) && $(CROSS_COMPILE)gcc -o evtest evtest.c
	cd $(REQUIRES_JSTEST) && $(CROSS_COMPILE)gcc -o jstest jstest.c
	cd $(REQUIRES_UNZIP60) && make -f unix/Makefile.trimuismart unzip
	cd $(REQUIRES_SDL2) && ( [ -f configure ] || ./autogen.sh ) && ./configure --host=$(CROSS_TRIPLE) --prefix=$(PREFIX_LOCAL) --enable-video-mali --enable-alsa --enable-video-opengles --disable-video-opengl --disable-video-x11 --disable-video-wayland --disable-video-kmsdrm --disable-pulseaudio --disable-jack --disable-esd --disable-arts --disable-nas --disable-sndio --disable-dbus --disable-ibus --disable-fcitx --disable-hidapi --disable-power --disable-filesystem --disable-sensor --disable-locale --disable-loadso --enable-shared --disable-static && make -j && make install
	mkdir -p $(REQUIRES_SDL2)/output
	cp -a $(PREFIX_LOCAL)/lib/libSDL2*.so* $(REQUIRES_SDL2)/output/
	cp -a $(PREFIX)/lib/libSDL2_image*.so* $(REQUIRES_SDL2)/output/ 2>/dev/null || true
	cp -a $(PREFIX)/lib/libSDL2_ttf*.so* $(REQUIRES_SDL2)/output/ 2>/dev/null || true
	cp -a $(PREFIX)/lib/libUMP.so* $(REQUIRES_SDL2)/output/
	cp -a $(PREFIX)/lib/libtinyalsa.so* $(REQUIRES_SDL2)/output/
	cp -a $(PREFIX)/lib/libasound.so* $(REQUIRES_SDL2)/output/
	cd boot && sh build.sh

clean:
	cd $(REQUIRES_FILEMGR) && make clean PLATFORM=tg5040
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
	git clone --depth 1 https://github.com/JohnnyonFlame/SDL-malifbdev-rot.git $(REQUIRES_SDL2)

include ../all/readmes/makefile
