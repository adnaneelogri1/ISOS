#ifndef DYNLOADER_H
#define DYNLOADER_H

#include "elf_parser.h"

void* my_dlopen(const char* library_path);
void* my_dlsym(void* handle, const char* symbol_name);
int check_elf(const char* library_path);
int validate_load_segments(const char* library_path, elf_header* hdr);

#endif