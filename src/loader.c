#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <unistd.h>
#include <stddef.h>
#include "loader.h"
#include "dynloader.h"
#include "debug.h"
#include "isos-support.h"

#define DT_NULL     0
#define DT_STRTAB   5
#define DT_SYMTAB   6

/**
 * @brief The function loader_plt_resolver() is used to resolve symbols
 * in the PLT (Procedure Linkage Table) section of a shared library.
 *
 * @param handle Pointer to the shared library handle.
 * @param sym_id The ID of the symbol to resolve.
 * @return Pointer to the function address if successful, NULL otherwise.
 * @brief The function isos_trampoline() is called by the PLT section
 * entry for each imported symbol inside the DL library.
*/

void *loader_plt_resolver(void *handle, int sym_id) {
    if (!handle) {
        debug_error("Invalid handle in PLT resolver");
        return NULL;
    }

    // Cast handle to our loader_info structure
    lib_handle_t *loader_info = (lib_handle_t *) handle;

    // Step 1: Get symbol name from ID using the imported symbols table
    const char *sym_name = get_symbol_name_by_id(loader_info->imported_symbols, sym_id);
    if (!sym_name) {
        debug_error("Could not resolve symbol name");
        return NULL;
    }

    debug_info("Resolving symbol name");

    // Step 2: Find function address by name in the exported symbols table
    void *func_addr = find_function_by_name(loader_info->plt_resolve_table, sym_name);
    if (!func_addr) {
        debug_error("Could not find function address");
        return NULL;
    }

    return func_addr;
}

/**
 * Initialise la bibliothèque en configurant les pointeurs loader_info
 *
 * @param handle Handle de la bibliothèque
 * @param plt_table Table des symboles pour la résolution PLT
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
/*
int init_library(void* handle, void* plt_table) {
    printf("Initialisation de la bibliothèque\n");
    if (!handle) {
        debug_error("Handle invalide");
        return -1;
    }

   lib_handle_t* lib = (lib_handle_t*)handle;
    if (lib->hdr.e_entry != 0) {
    // Calculer l'adresse absolue de loader_info
    loader_info_t* info = (loader_info_t*)((char*)lib->base_addr + lib->hdr.e_entry);

    // Afficher des infos pour déboguer
    printf("loader_info à l'adresse: %p\n", info);

    // Configurer les pointeurs
    if (info->loader_handle) {
        printf("Mise à jour de loader_handle à l'adresse: %p\n", info->loader_handle);
        *(info->loader_handle) = handle;
    }

    if (info->isos_trampoline) {
        printf("Mise à jour de isos_trampoline à l'adresse: %p\n", info->isos_trampoline);
        *(info->isos_trampoline) = isos_trampoline;
    }

    // Configurer la table PLT
    return 0;
}
}*/
const char *get_symbol_name_by_id(const char **imported_symbols, int sym_id) {
    if (!imported_symbols) {
        debug_error("Imported symbols table is NULL");
        return NULL;
    }

    // Check if symbol ID is valid
    if (sym_id < 0) {
        debug_error("Invalid symbol ID");
        return NULL;
    }

    return imported_symbols[sym_id];
}

// Function to find function address by name in the resolve table
void *find_function_by_name(symbol_entry *exported_symbols, const char *name) {
    if (!exported_symbols || !name) {
        debug_error("Invalid exported symbols table or name");
        return NULL;
    }

    // Search for the symbol in the table
    int i = 0;
    while (exported_symbols[i].name != NULL) {
        if (strcmp(exported_symbols[i].name, name) == 0) {
            debug_detail("Found function address for symbol");
            return exported_symbols[i].addr;
        }
        i++;
    }

    debug_error("Symbol not found in exported symbols table");
    return NULL;
} // The main PLT resolver function


//foo 
//coment les fonctionnnes
//ce qui fait projet 
//chacun des challs ,l e but de chall et commetnj'ai rosule
//
