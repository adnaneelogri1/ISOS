#include "../include/dynloader.h"
#include <dlfcn.h>
#include <stdio.h>

void* my_dlopen(const char* library_path) {
    void* handle = dlopen(library_path, RTLD_LAZY);
        if (handle == NULL) {
        fprintf(stderr, "Error: can't load %s: %s\n", library_path, dlerror());
    }
    
    return handle;
}

void* my_dlsym(void* handle, const char* symbol_name) {
    dlerror();
    
    void* symbol = dlsym(handle, symbol_name);
    
    char* error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Error: can't find %s: %s\n", symbol_name, error);
        return NULL;
    }
    
    return symbol;
}
