ifeq (,$(CROSS_COMPILE))
$(error missing CROSS_COMPILE for this toolchain)
endif

TARGET = rfkill
PRODUCT = $(TARGET).elf

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
FLAGS	= -Os -lrt -ldl -Wl,--gc-sections -s

all:
	$(CXX) $(TARGET).cpp -o $(PRODUCT) $(FLAGS)
clean:
	rm -rf $(PRODUCT)