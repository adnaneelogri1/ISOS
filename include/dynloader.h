#ifndef DYNLOADER_H
#define DYNLOADER_H

#include "elf_parser.h"

/**
 * Loads a dynamic library 
 * Returns NULL on error
 */
void* my_dlopen(const char* library_path);

/**
 * Gets a symbol from a loaded library
 * Returns NULL on error
 */
void* my_dlsym(void* handle, const char* symbol_name);

/**
 * Check if file is a valid ELF binary
 * Returns 0 on success, -1 on error
 */
int check_elf(const char* library_path);

/**
 * Validate PT_LOAD segments in an ELF binary
 * Returns 0 on success, -1 on error
 */
int validate_load_segments(const char* library_path, elf_header* hdr);

#endif