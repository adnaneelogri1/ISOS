# 
# This Makefile is for the ISOS project
#

INCLUDE_DIR= ./include

SRC_FILES=./src/loader.c ./src/utils.c ./src/elf_parser.c ./src/load_library.c ./src/relocation.c ./src/debug.c ./src/symbol_parser.c
 
# Compiler settings
CC = gcc
CFLAGS = -Wall -I$(INCLUDE_DIR) -g  
LDFLAGS = -shared -nostdlib -O3 

# Main targets
all: libmylib.so libmylib_hidden.so isos_loader

# Simple shared library
libmylib.so: src/mylib.c
	$(CC) $(CFLAGS) $(LDFLAGS) -e loader_info -o $@ $<

# Shared library with hidden visibility
libmylib_hidden.so: src/mylib.c
	$(CC) $(CFLAGS) $(LDFLAGS)  -shared -fvisibility=hidden -e loader_info -fPIC   -o $@ $<
# Compilation des fichiers sources en objets
loader.o: src/loader.c
	$(CC) $(CFLAGS) -c -o $@ $<

utils.o: src/utils.c
	$(CC) $(CFLAGS) -c -o $@ $<

elf_parser.o: src/elf_parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

load_library.o: src/load_library.c
	$(CC) $(CFLAGS) -c -o $@ $<

relocation.o: src/relocation.c
	$(CC) $(CFLAGS) -c -o $@ $<

debug.o: src/debug.c
	$(CC) $(CFLAGS) -c -o $@ $<

symbol_parser.o: src/symbol_parser.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Main program
isos_loader: loader.o utils.o elf_parser.o load_library.o relocation.o debug.o symbol_parser.o
	$(CC) -o $@ $^

# Run tests
test:
	./test/elf_parser.sh 

# Clean up
clean:
	rm -f *.o *.so isos_loader

.PHONY: all clean test