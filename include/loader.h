#ifndef LOADER_H
#define LOADER_H

// Structure pour notre table de symboles personnalisée
typedef struct {
    const char* name;
    void* addr;
} symbol_entry;
// Structure for the loader
typedef struct {
    // Exported symbols table
    symbol_entry* exported_symbols;
    const char** imported_symbols;

    // Handle to be passed to libraries
    void** loader_handle;
    
    // Pointer to the resolver function
    void** isos_trampoline;
    

} loader_info_t;
#endif