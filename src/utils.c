#include "../include/dynloader.h"
#include "../include/elf_parser.h"
#include <dlfcn.h>
#include <stdio.h>

int check_elf(const char* library_path) {
    elf_header hdr;
    
    // Read the header
    if (read_elf_header(library_path, &hdr) != 0) {
        return -1;
    }
    
    // Make sure it's a valid library
    if (check_valid_lib(&hdr) != 0) {
        return -1;
    }
    
    // Enable this for debugging
    print_header(&hdr);
    
    return 0;
}

void* my_dlopen(const char* library_path) {
    // Check if it's a valid ELF shared library
    if (check_elf(library_path) != 0) {
        perror( "Error: is not a valid shared library\n");
        return NULL;
    }
    
    // Now load it
    void* handle = dlopen(library_path, RTLD_LAZY);
    if (handle == NULL) {
        perror( "Error: can't load : \n");
    }
    
    return handle;
}

void* my_dlsym(void* handle, const char* symbol_name) {
    dlerror();
    
    void* symbol = dlsym(handle, symbol_name);
    
    char* error = dlerror();
    if (error != NULL) {
        perror("Error: can't find : \n");
        return NULL;
    }
    
    return symbol;
}