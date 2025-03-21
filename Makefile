# 
# This Makefile is for the ISOS project and ensure compatibility with the CI.
# Make sure to include this file in your root Makefile (i.e., at the top-level of your repository).
#


INCLUDE_DIR=./include

SRC_FILES=./src/loader.c ./src/utils.c

# TODO
# Uncomment this and initialize it to the correct path(s) to your source files if your project sources are not located in `src`.
#vpath %.c path/to/src

# Compiler settings
CC = gcc
CFLAGS = -Wall -I$(INCLUDE_DIR)
LDFLAGS = -shared

# Main targets
all: libmylib.so isos_loader

# First test with a standard shared library (no -nostdlib)
libmylib.so: src/mylib.c
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

# Object files
utils.o: src/utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

loader.o: src/loader.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Main program
isos_loader: loader.o utils.o
	$(CC) -o $@ $^ -ldl

# Clean up
clean:
	rm -f *.o *.so isos_loader

.PHONY: all clean
