# 
# This Makefile is for the ISOS project and ensure compatibility with the CI.
# Make sure to include this file in your root Makefile (i.e., at the top-level of your repository).
#

# Initialize this variable to point to the directory holding your header
INCLUDE_DIR=./include

# Directory for object files
OBJ_DIR=./obj

# Source files for the loader
SRC_FILES=$(wildcard ./src/*.c)

# Object files with path in obj directory
OBJ_FILES=$(patsubst ./src/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# compiler
CC=gcc 

# compiler flags with -rdynamic 
CFLAGS := -g -Wall -Wextra -I$(INCLUDE_DIR) 
LDFLAGS := -rdynamic

all: $(OBJ_DIR) isos_loader libmylib.so

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

libmylib.so: src/mylib.c
	$(CC) -shared -I $(INCLUDE_DIR) $^ --entry loader_info -o $@ -fvisibility=hidden

isos_loader: $(OBJ_FILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Rule to compile .clibmylib files from src to .o files in obj
$(OBJ_DIR)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test:
	./test/elf_parser.sh

clean:
	rm -f isos_loader libmylib.so
	rm -rf $(OBJ_DIR)

.PHONY: clean test all