# Makefile for PirateProto 2 Flipper Zero Application

# Compiler and flags
CC = gcc
AR = ar
STRIP = strip

# Basic compilation flags (ARM flags removed for host compilation)
CFLAGS = -Wall -Wextra -std=gnu11 -Os -fno-common -ffunction-sections -fdata-sections
CFLAGS += -fshort-enums -fno-strict-aliasing
CFLAGS += -I. -I./pirateproto2 -I./pirateproto2/helpers -I./pirateproto2/protocols
CFLAGS += -I./pirateproto2/scenes -I./pirateproto2/views

# Linker flags (ARM flags removed for host compilation)
LDFLAGS = -Wl,--gc-sections -Wl,-Map=output.map

# Source directories
SRC_DIR = pirateproto2
HELPERS_DIR = $(SRC_DIR)/helpers
PROTOCOLS_DIR = $(SRC_DIR)/protocols
SCENES_DIR = $(SRC_DIR)/scenes
VIEWS_DIR = $(SRC_DIR)/views

# Source files
SRCS = $(SRC_DIR)/pirateproto2_app.c
SRCS += $(HELPERS_DIR)/pirateproto2_storage.c
SRCS += $(HELPERS_DIR)/radio_device_loader.c
SRCS += $(PROTOCOLS_DIR)/protocol_items.c
SRCS += $(PROTOCOLS_DIR)/ford_v0.c
SRCS += $(PROTOCOLS_DIR)/fiat_v0.c
SRCS += $(PROTOCOLS_DIR)/kia_v0.c
SRCS += $(PROTOCOLS_DIR)/kia_v1.c
SRCS += $(PROTOCOLS_DIR)/kia_v2.c
SRCS += $(PROTOCOLS_DIR)/kia_v3_v4.c
SRCS += $(PROTOCOLS_DIR)/kia_v5.c
SRCS += $(PROTOCOLS_DIR)/subaru.c
SRCS += $(PROTOCOLS_DIR)/suzuki.c
SRCS += $(PROTOCOLS_DIR)/vw.c
SRCS += $(SCENES_DIR)/pirateproto2_scene.c
SRCS += $(SCENES_DIR)/protopirate_scene_start.c
SRCS += $(SCENES_DIR)/protopirate_scene_receiver.c
SRCS += $(SCENES_DIR)/protopirate_scene_receiver_info.c
SRCS += $(SCENES_DIR)/protopirate_scene_saved.c
SRCS += $(SCENES_DIR)/protopirate_scene_saved_info.c
SRCS += $(SCENES_DIR)/protopirate_scene_emulate.c
SRCS += $(SCENES_DIR)/protopirate_scene_about.c
SRCS += $(VIEWS_DIR)/protopirate_receiver.c
SRCS += $(VIEWS_DIR)/protopirate_receiver_info.c

# Object files
OBJS = $(SRCS:.c=.o)

# Output
TARGET = pirateproto2.fap
LIBTARGET = libpirateproto2.a

# Default target
all: $(TARGET)

# Build the library
$(LIBTARGET): $(OBJS)
	$(AR) rcs $@ $^

# Build the FAP file (simplified - normally requires Flipper SDK)
$(TARGET): $(LIBTARGET)
	@echo "Creating $(TARGET) (requires Flipper SDK for complete build)"
	@echo "For full build, use: docker run flipperdevices/flipperzero-dev fbt fap_pirateproto2"
	@# Create a placeholder file
	@echo "PirateProto 2 FAP File" > $(TARGET)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(LIBTARGET) $(TARGET) output.map

# Show help
help:
	@echo "PirateProto 2 Build System"
	@echo "Targets:"
	@echo "  all     - Build the FAP file (default)"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Note: Complete build requires Flipper Zero SDK"
	@echo "Use Docker: docker run flipperdevices/flipperzero-dev fbt fap_pirateproto2"

.PHONY: all clean help
