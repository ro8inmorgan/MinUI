CORES= a2600
CORES+= a5200
CORES+= a7800
CORES+= handy
CORES+= c64
CORES+= c128
CORES+= fake-08
CORES+= fbneo
CORES+= fceumm
CORES+= gambatte
CORES+= gpsp
CORES+= libretro-cap32
CORES+= libretro-uae
CORES+= mednafen_pce_fast
CORES+= mednafen_supafaust
CORES+= mednafen_vb
CORES+= mgba
CORES+= pcsx_rearmed
CORES+= pet
CORES+= picodrive
CORES+= plus4
CORES+= pokemini
CORES+= race
CORES+= snes9x
CORES+= vic
CORES+= prboom
CORES+= bluemsx
CORES+= gearcoleco

###############################

# optional core vars
# *_REPO=
# *_HASH=
# *_CORE=
# *_FLAGS=
# *_MAKEFILE=
# *_BUILD_PATH=
# *_SUBMODULES=

a2600_CORE = stella2014_libretro.so

a5200_CORE = a5200_libretro.so

a7800_CORE = prosystem_libretro.so


fbneo_CORE = fbneo_libretro.so
fbneo_BUILD_PATH = fbneo/src/burner/libretro
fbneo_MAKE = make


fake-08_CORE = fake08_libretro.so
fake-08_BUILD_PATH = fake-08/platform/libretro




# mGBA replaced Makefile.libretro with a CMake-based libretro build.
mgba_CORE = build-tg5050/mgba_libretro.so
mgba_MAKE = cmake -S . -B build-tg5050 -DCMAKE_BUILD_TYPE=Release -DLIBMGBA_ONLY=ON -DBUILD_LIBRETRO=ON -DLIBRETRO_STATIC=OFF -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DCMAKE_C_COMPILER=$(CROSS_COMPILE)gcc -DCMAKE_CXX_COMPILER=$(CROSS_COMPILE)g++ -DCMAKE_C_FLAGS="-fomit-frame-pointer -ffast-math -mcpu=cortex-a55 -fno-common -ftree-vectorize -funswitch-loops" -DCMAKE_CXX_FLAGS="-fomit-frame-pointer -ffast-math -mcpu=cortex-a55 -fno-common -ftree-vectorize -funswitch-loops" && cmake --build build-tg5050 --target mgba_libretro -- $(PROCS)

pcsx_rearmed_MAKEFILE = Makefile.libretro
# pcsx has a disabled submodule for gnulib, which takes forever to check out and is unused - skil checkout
pcsx_rearmed_SUBMODULES = frontend/libpicofe

picodrive_MAKEFILE = Makefile.libretro
# No background Audio on Sega CD
# https://github.com/LoveRetro/NextUI/issues/68

pokemini_MAKEFILE = Makefile.libretro
# Our patchfile doesn't work on latest Pokemon mini commits, pinning to last working hash for now


snes9x2005_plus_FLAGS = USE_BLARGG_APU=1

snes9x_CORE = snes9x_libretro.so
snes9x_BUILD_PATH = snes9x/libretro

libretro-uae_CORE = puae2021_libretro.so

libretro-cap32_CORE = cap32_libretro.so

c64_CORE  = vice_x64_libretro.so
c64_FLAGS = EMUTYPE=x64

c128_CORE  = vice_x128_libretro.so
c128_FLAGS = EMUTYPE=x128

vic_CORE  = vice_xvic_libretro.so
vic_FLAGS = EMUTYPE=xvic

pet_CORE  = vice_xpet_libretro.so
pet_FLAGS = EMUTYPE=xpet

plus4_CORE  = vice_xplus4_libretro.so
plus4_FLAGS = EMUTYPE=xplus4

bluemsx_CORE = bluemsx_libretro.so
bluemsx_MAKEFILE = Makefile.libretro

gearcoleco_CORE = gearcoleco_libretro.so
gearcoleco_BUILD_PATH = gearcoleco/platforms/libretro

###############################

include ../../all/cores/makefile
