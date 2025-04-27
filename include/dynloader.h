#ifndef DYNLOADER_H
#define DYNLOADER_H

#include "elf_parser.h"
#include "loader.h"

void* my_dlopen(const char* library_path);
void* my_dlsym(void* handle, const char* symbol_name);
int check_elf(const char* library_path);
int validate_load_segments(const char* library_path, elf_header* hdr);



typedef struct {
    void* base_addr;
    void* plt_resolve_table;
    // Exported symbols table
    symbol_entry* imported_symbols;
    symbol_entry* exported_symbols;
} lib_handle_t;

int my_set_plt_resolve(void* handle, void* resolve_table);

#endif