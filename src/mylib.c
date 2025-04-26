#include <stddef.h>
#include "mylib.h"
#include "isos-support.h"
#include "loader.h"

// Create PLT entries for imported functions
PLT_BEGIN
PLT_ENTRY(0, new_foo)
PLT_ENTRY(1, new_bar)
// Symbols needed by PLT_BEGIN - will be set by the loader
void *loader_handle = NULL;
void *isos_trampoline = NULL;

    const char* new_bar();
    const char* new_foo();
// Integer identifiers for imported symbols
#define SYM_NEW_FOO 0
#define SYM_NEW_BAR 1
// Imported symbol table - array of strings indexed by symbol ID
const char* imported_symbols[] = {
    "new_foo", // ID 0 - SYM_NEW_FOO
    "new_bar"  // ID 1 - SYM_NEW_BAR
};


// Définition des fonctions
const char* foo_exported() {
    return "Hello from foo_exported()";
}

const char* bar_exported() {
    return "Hello from bar_exported()";
}

// Fonction cachée (sera hidden avec -fvisibility=hidden)
const char* hidden_func() {
    return "Hello from hidden_func()";
}

// Notre table de symboles exportée
symbol_entry exported_symbols[] = {
    {"foo_exported", (void*)foo_exported},
    {"bar_exported", (void*)bar_exported},
    {"hidden_func", (void*)hidden_func},
    {NULL, NULL} // Marque la fin de la table
};


// Initialize the loader info
loader_info_t loader_info = {
    .exported_symbols = exported_symbols,
    .imported_symbols = imported_symbols,
    .loader_handle = &loader_handle,
    .isos_trampoline = &isos_trampoline
};

// Cette fonction sera utilisée comme point d'entrée (e_entry)
symbol_entry* get_symbol_table() {
    return exported_symbols;
}

// Fonctions qui importent depuis le loader
const char* bar_imported() {
    // Force use of hidden_func to prevent optimization
    //const char* temp = hidden_func(); // Use it to prevent it from being optimized out
    return new_bar();       
}

const char* foo_imported() {
    return new_foo();
}


