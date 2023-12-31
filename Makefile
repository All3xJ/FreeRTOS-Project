OUTPUT_DIR := ./output
IMAGE := RTOSDemo.out
SUB_MAKEFILE_DIR = ./library-makefiles

# Change this lines with your compiler
CC = /Applications/ARM/bin/arm-none-eabi-gcc-10.3.1
LD = /Applications/ARM/bin/arm-none-eabi-gcc-10.3.1
SIZE = /Applications/ARM/bin/arm-none-eabi-size
MAKE = make

CFLAGS += $(INCLUDE_DIRS) -nostartfiles -ffreestanding -mthumb -mcpu=cortex-m3 \
		  -Wall -Wextra -g3 -O0 -ffunction-sections -fdata-sections \
		  -MMD -MP -MF"$(@:%.o=%.d)" -MT $@

INCLUDE_DIRS += -I headers \
                -I source \
		-I /Applications/ARM/arm-none-eabi/include/c++/10.3.1 \ # Change this line with the correct binary (if needed)

# Source files
SOURCE_DIR := source
SOURCE_FILES := $(wildcard $(SOURCE_DIR)/*.c)

# Main file
MAIN_FILE := main.c

# Create a list of object files with the desired output directory path.
OBJS_OUTPUT = $(patsubst $(SOURCE_DIR)/%.c,$(OUTPUT_DIR)/%.o,$(SOURCE_FILES))
OBJS_OUTPUT += $(OUTPUT_DIR)/main.o

# Create a list of dependency files with the desired output directory path.
DEP_OUTPUT = $(patsubst $(SOURCE_DIR)/%.c,$(OUTPUT_DIR)/%.d,$(SOURCE_FILES))
DEP_OUTPUT += $(OUTPUT_DIR)/main.d

# Create the output directory if it doesn't exist
$(shell mkdir -p $(OUTPUT_DIR))

all: $(OUTPUT_DIR)/$(IMAGE)

$(OUTPUT_DIR)/%.o: $(SOURCE_DIR)/%.c $(OUTPUT_DIR)/%.d Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR)/main.o: $(MAIN_FILE) $(OUTPUT_DIR)/main.d Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_DIR)/$(IMAGE): ./mps2_m3.ld $(OBJS_OUTPUT) Makefile
	@echo ""
	@echo ""
	@echo "--- Final linking ---"
	@echo ""
	$(LD) $(OBJS_OUTPUT) $(CFLAGS) -Xlinker --gc-sections -Xlinker -T ./mps2_m3.ld \
		-Xlinker -Map=$(OUTPUT_DIR)/RTOSDemo.map -specs=nano.specs \
		-specs=nosys.specs -specs=rdimon.specs -o $(OUTPUT_DIR)/$(IMAGE)
	$(SIZE) $(OUTPUT_DIR)/$(IMAGE)

$(DEP_OUTPUT):
include $(wildcard $(DEP_OUTPUT))

clean:
	rm -rf $(OUTPUT_DIR)

.PHONY: all clean
