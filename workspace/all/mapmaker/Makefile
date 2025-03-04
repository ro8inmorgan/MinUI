ifeq (,$(PLATFORM))
	PLATFORM=$(UNION_PLATFORM)
endif

ifeq (,$(PLATFORM))
	$(error please specify PLATFORM, eg. PLATFORM=trimui make)
endif

# Define the target name
TARGET = mapmaker

# Define the Python script
PYTHON_SCRIPT = mapmaker.py

# Define the output directory of PyInstaller
DIST_DIR = dist

# Define the output executable
OUTPUT_EXECUTABLE = $(DIST_DIR)/$(TARGET)

# Define the final location to copy the executable
FINAL_LOCATION = ./build/$(PLATFORM)/$(TARGET)

# Phony targets
.PHONY: all clean

# Default target
all: build_dir $(TARGET)

# Ensure build directory exists
build_dir:
	mkdir -p ./build/$(PLATFORM)

# Build target
$(TARGET):
	pyinstaller --onefile $(PYTHON_SCRIPT)
	cp $(OUTPUT_EXECUTABLE) $(FINAL_LOCATION)
	rm -rf $(DIST_DIR) $(TARGET).spec

# Clean target
clean:
	rm -rf build $(DIST_DIR) $(TARGET).spec
