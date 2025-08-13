# Makefile for xml.h example

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -O2 -D_GNU_SOURCE

# Target executable
TARGET = example
SOURCE = example.c

# Default target
all: $(TARGET)

# Build the example
$(TARGET): $(SOURCE) xml.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TARGET).exe

# Mark targets as phony
.PHONY: all clean
