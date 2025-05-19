# Makefile for Virtual Machine
# This Makefile compiles the virtual machine code with appropriate flags and settings
# Works on both Windows and Unix-like systems

# Detect operating system
ifeq ($(OS),Windows_NT)
    DETECTED_OS := Windows
    # Define Windows-specific commands
    RM := del /Q
    RM_DIR := rmdir /S /Q
    # Fix path separator for Windows
    PATHSEP := \\
else
    DETECTED_OS := $(shell uname -s)
    # Define Unix-like commands
    RM := rm -f
    RM_DIR := rm -rf
    PATHSEP := /
endif

# Print detected OS (uncomment for debugging)
# $(info Detected OS: $(DETECTED_OS))

# Compiler and flags
CC = gcc
# -Wall: Enable all common warnings
# -Wextra: Enable additional warnings
# -std=c11: Use C11 standard
# -g: Include debugging information
CFLAGS = -Wall -Wextra -std=c11 -g

# Name of the output executable
ifeq ($(DETECTED_OS),Windows)
    TARGET = vm.exe
else
    TARGET = vm
endif

# Source files
SOURCES = main.c
HEADERS = main.h

# Object files derived from source files
OBJECTS = $(SOURCES:.c=.o)

# Default target (first target is the default)
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CC) $(CFLAGS) -o $@ $^

# Rule to compile source files into object files
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up intermediate build files and the executable
clean:
	@echo "Cleaning up..."
ifeq ($(DETECTED_OS),Windows)
	@echo "Using Windows cleanup commands..."
	@if exist $(subst /,$(PATHSEP),$(OBJECTS)) $(RM) $(subst /,$(PATHSEP),$(OBJECTS))
	@if exist $(TARGET) $(RM) $(TARGET)
else
	@echo "Using Unix cleanup commands..."
	$(RM) $(OBJECTS) $(TARGET)
endif

# Rebuild everything from scratch
rebuild: clean all

# Run the program
# Modified to accept image file path as an optional argument
run: $(TARGET)
	@echo "Running $(TARGET)..."
ifeq ($(IMAGE),)
	@echo "No image file specified. Use 'make run IMAGE=path/to/image.obj' to provide an image file."
	@echo "For now, creating a dummy test image file..."
	@echo "0000" > test_image.obj
	./$(TARGET) test_image.obj
else
	./$(TARGET) $(IMAGE)
endif

# Check for memory leaks (platform-specific)
memcheck: $(TARGET)
ifeq ($(DETECTED_OS),Windows)
	@echo "Memory checking on Windows requires external tools like Dr. Memory"
	@echo "Example: drmemory -light -- ./$(TARGET)"
else
	@command -v valgrind >/dev/null 2>&1 && \
		echo "Running valgrind..." && \
		valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) || \
		echo "Valgrind not installed. Skipping memory check."
endif

# Help target explains available make commands
help:
	@echo "Available targets:"
	@echo "  all       - Build the executable (default)"
	@echo "  clean     - Remove object files and executable"
	@echo "  rebuild   - Clean and rebuild everything"
	@echo "  run       - Build and run the program (use 'make run IMAGE=path/to/image.obj' to specify an image)"
	@echo "  memcheck  - Run with memory checker (Valgrind on Unix, Dr. Memory on Windows)"
	@echo "  help      - Show this help message"

# Phony targets - these don't represent files
.PHONY: all clean rebuild run memcheck help