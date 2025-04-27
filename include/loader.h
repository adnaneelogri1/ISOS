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
int init_library(void* handle, void* plt_table);
const char* get_symbol_name_by_id(const char** imported_symbols, int sym_id) ;
void* find_function_by_name(symbol_entry* exported_symbols, const char* name) ;
#endif