CMAKE := cmake
CMAKE_BUILD_TYPE := debug
# CMAKE_BUILD_TYPE can be set to "debug", "release", "relwithdeb
CMAKE_BUILD_DIR := build

CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)


.PHONY: all clean upload debug

OPENOCD_DIR ?=E:/environment/xpack-openocd-0.12.0-3-win32-x64/xpack-openocd-0.12.0-3/openocd
OPENOCD_TARGET :=$(OPENOCD_DIR)/scripts/target/stm32f4x.cfg
OPENOCD_INTERFACE :=$(OPENOCD_DIR)/scripts/interface/stlink.cfg

ELF_FILE := $(CMAKE_BUILD_DIR)/main.elf
BIN_FILE := $(CMAKE_BUILD_DIR)/main.bin
HEX_FILE := $(CMAKE_BUILD_DIR)/main.hex

all: generate build

build : $(CMAKE_BUILD_DIR)/CMakeLists.txt
	@echo "Building project with CMake..."
	$(CMAKE) --build $(CMAKE_BUILD_DIR) --parallel $(shell nproc)

generate: $(CMAKE_BUILD_DIR)/CMakeLists.txt

bin: $(BIN_FILE)

hex: $(HEX_FILE)

$(BIN_FILE): $(ELF_FILE)
	arm-none-eabi-objcopy -O binary $< $@

$(HEX_FILE): $(ELF_FILE)
	arm-none-eabi-objcopy -O ihex $< $@

$(CMAKE_BUILD_DIR)/CMakeLists.txt : CMakeLists.txt
	@echo "Configuring project with CMake..."
	mkdir -p $(CMAKE_BUILD_DIR)
	$(CMAKE) -S . -B $(CMAKE_BUILD_DIR) $(CMAKE_FLAGS)

clean:
	rm -rf $(CMAKE_BUILD_DIR)

	
upload:$(HEX_FILE) $(BIN_FILE)
	openocd -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET) -c "program $(HEX_FILE)" -c "reset" \
	-c "shutdown" 

debug:
	openocd -f $(OPENOCD_INTERFACE) -f $(OPENOCD_TARGET)