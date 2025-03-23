#ifndef DYNLOADER_H
#define DYNLOADER_H

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
int check_elf(const char* library_path);

#endif 
