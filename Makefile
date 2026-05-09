# Target MCU and clock. F_CPU is used by avr-libc delay helpers.
MCU = attiny4313
PROGRAMMER = usbasp
F_CPU = 20000000UL

# Output firmware name: build/firmware.elf and build/firmware.hex.
TARGET = firmware

# Project source files. Paths are mirrored inside build/.
SRC = \
	main.cpp \
	lib/disp/disp_class.cpp \
	lib/pwm/rgbw_pwm.cpp \
	lib/usart/usart.cpp

# AVR toolchain commands.
CXX = avr-g++
CC = avr-gcc
OBJCOPY = avr-objcopy
SIZE = avr-size
AVRDUDE = avrdude

# Compiler flags:
# -mmcu selects the chip, -DF_CPU sets clock for delays, -Os keeps firmware small.
CXXFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -std=c++17 -I.
CXXFLAGS += -Wall -Wextra

# Header search paths for project libraries.
CXXFLAGS += -Ilib/disp
CXXFLAGS += -Ilib/pwm
CXXFLAGS += -Ilib/usart

# Linker target MCU must match compiler target MCU.
LDFLAGS = -mmcu=$(MCU)

BUILD_DIR = build

# Convert each .cpp path into a matching .o path under build/.
OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR)/$(TARGET).hex

# Create the root build directory.
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile one source file into one object file.
# $(@D) creates nested directories such as build/lib/disp.
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link all objects into an ELF file.
$(BUILD_DIR)/$(TARGET).elf: $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@

# Convert ELF to Intel HEX for flashing and print firmware size.
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@
	$(SIZE) $<

# Flash the HEX file through USBasp.
flash: $(BUILD_DIR)/$(TARGET).hex
	$(AVRDUDE) -c $(PROGRAMMER) -p t4313 -U flash:w:$<:i

# Remove all generated build files.
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all flash clean
