# 
# This Makefile is for the ISOS project and ensure compatibility with the CI.
# Make sure to include this file in your root Makefile (i.e., at the top-level of your repository).
#

# TODO
# Initialize this variable to point to the directory holding your header if any.
# Otherwise, the CI will consider the top-level directory.
INCLUDE_DIR=./include


# TODO
# Initialize this variable with a space separated list of the paths to the loader source files (not the library).
# You can use some make native function such as wildcard if you want.
SRC_FILES=$(wildcard ./src/*.c)



# TODO
# Uncomment this and initialize it to the correct path(s) to your source files if your project sources are not located in `src`.
#vpath %.c path/to/src


# compiler
CC=gcc 

# compiler flags with -rdynamic 
CFLAGS   := -g -Wall -Wextra -I$(INCLUDE_DIR)
LDFLAGS  := -rdynamic
all: isos_loader  src/mylib.so


src/mylib.so: src/mylib.c
	
	gcc -shared -I ./include $^ --entry loader_info  -o $@ -fvisibility=hidden

	
isos_loader : $(SRC_FILES:.c=.o)
	$(CC)  $(CFLAGS) -o $@ $^ 


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f isos_loader  $(wildcard ./src/*.o)

.PHONY: clean