# Compiler
CC = gcc

# Source file
SRC = ptmalloc-structs.c

# Output directory
OUT_DIR = build

# Common compiler flags
CFLAGS = -g -fno-eliminate-unused-debug-types

# GLIBC version range
GLIBC_MIN_VERSION = 212
GLIBC_MAX_VERSION = 239

# Define target names for each architecture
TARGETS = i386 x86_64

# Determine the architecture-specific flags
ARCH_FLAGS_i386 = -m32
ARCH_FLAGS_x86_64 =

# Default target
all: $(TARGETS)

# Ensure the output directory exists
$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Pattern rule for compiling with different architectures
$(TARGETS): %: $(OUT_DIR)
	@for version in $(shell seq $(GLIBC_MIN_VERSION) $(GLIBC_MAX_VERSION)); do \
		GLIBC_VERSION=$$version; \
		GLIBC_VERSION_FORMATTED=$${GLIBC_VERSION#0}; \
		$(CC) -DGLIBC_VERSION=$$GLIBC_VERSION_FORMATTED -c $(SRC) $(CFLAGS) $(ARCH_FLAGS_$@) -o $(OUT_DIR)/ptmalloc-structs-$@-$$GLIBC_VERSION_FORMATTED.debug; \
	done

# Clean target
clean:
	rm -rf $(OUT_DIR)

.PHONY: all clean $(TARGETS)
