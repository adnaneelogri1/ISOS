
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
 * @brief The function isos_trampoline() is called by the PLT section
 * entry for each imported symbol inside the DL library.
*/
void isos_trampoline();
asm(".pushsection .text,\"ax\",\"progbits\""  "\n"
    "isos_trampoline:"                        "\n"
    POP_S(REG_ARG_1)                          "\n"
    POP_S(REG_ARG_2)                          "\n"
    PUSH_STACK_STATE                          "\n"
    CALL(loader_plt_resolver)                 "\n"
    POP_STACK_STATE                           "\n"
    JMP_REG(REG_RET)                          "\n"
    ".popsection"                             "\n");


/**
 * @param handler  : the loader handler returned by my_dlopen().
 * @param import_id: the identifier of the function to be called
 *                   from the imported symbol table.
 * @return the address of the function to be called by the trampoline.
*/
/**
 * @param handler  : the loader handler returned by my_dlopen().
 * @param import_id: the identifier of the function to be called
 *                   from the imported symbol table.
 * @return the address of the function to be called by the trampoline.
*/
void* loader_plt_resolver(void* handle, int sym_id) {
    if (!handle) {
        debug_error("Invalid handle in PLT resolver");
        return NULL;
    }
    
    // Cast handle to our loader_info structure
    lib_handle_t* loader_info = (lib_handle_t*)handle;
    
    // Step 1: Get symbol name from ID using the imported symbols table
    const char* sym_name = get_symbol_name_by_id(loader_info->imported_symbols, sym_id);
    if (!sym_name) {
        debug_error("Could not resolve symbol name");
        return NULL;
    }
    
    debug_info("Resolving symbol name");
    
    // Step 2: Find function address by name in the exported symbols table
    void* func_addr = find_function_by_name(loader_info->plt_resolve_table, sym_name);
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
int init_library(void* handle, void* plt_table) {
    printf("Initialisation de la bibliothèque\n");
    if (!handle) {
        debug_error("Handle invalide");
        return -1;
    }
    
    lib_handle_t* lib = (lib_handle_t*)handle;
    void* loader_info_addr = NULL;
    
    // Rechercher la structure loader_info dans la bibliothèque
    if (find_dynamic_symbol(lib->base_addr, &lib->hdr, lib->phdrs, 
                         "loader_info", &loader_info_addr) != 0 || !loader_info_addr) {
        debug_warn("Structure loader_info non trouvée");
        return -1;
    }
    
    // Configurer les pointeurs dans la structure loader_info
    loader_info_t* info = (loader_info_t*)loader_info_addr;
    
    // Configurer le handle du loader
    if (info->loader_handle) {
        *(info->loader_handle) = handle;
        debug_detail("Handle configuré dans loader_info");
    }
    
    // Configurer le trampoline
    if (info->isos_trampoline) {
        *(info->isos_trampoline) = isos_trampoline;
        debug_detail("Trampoline configuré dans loader_info");
    }
    
    // Configurer la table de résolution PLT
    if (plt_table) {
        lib->plt_resolve_table = plt_table;
        debug_detail("Table de résolution PLT configurée");
    }
    
    return 0;
}
// Function to get symbol name from ID
const char* get_symbol_name_by_id(const char** imported_symbols, int sym_id) {
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
void* find_function_by_name(symbol_entry* exported_symbols, const char* name) {
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
}// The main PLT resolver function
int find_dynamic_symbol(void* base_addr, elf_header* hdr, elf_phdr* phdrs, 
                    const char* name, void** symbol_addr) {
    // Vérif de base
    if (!base_addr || !hdr || !name || !symbol_addr) {
        return -1;
    }
    
    // Si le point d'entrée est valide
    if (hdr->e_entry != 0) {
        // Définir notre structure symbole
        typedef struct {
            const char* name;
            void* addr;
        } symbol_entry;
        
        // Définir le type de get_symbol_table
        typedef symbol_entry* (*get_table_func)();
        
        // Calculer l'adresse de get_symbol_table
        get_table_func get_table = (get_table_func)((char*)base_addr + hdr->e_entry);
        
        // Appeler la fonction pour obtenir la table
        symbol_entry* table = NULL;
        
        // Essayer d'appeler get_table - attention au segfault potentiel
        table = get_table();
        
        if (table) {
            // Parcourir la table des symboles
            for (int i = 0; table[i].name != NULL; i++) {
                if (strcmp(table[i].name, name) == 0) {
                    // Symbole trouvé, mais adresse peut être relative
                    if ((uintptr_t)table[i].addr < (uintptr_t)base_addr) {
                        // Adresse est un offset
                        *symbol_addr = (void*)((char*)base_addr + (uintptr_t)table[i].addr);
                    } else {
                        // Adresse déjà absolue
                        *symbol_addr = table[i].addr;
                    }
                    return 0;
                }
            }
        }
    }
    
    // Si on arrive ici, essayer avec la méthode standard (segment dynamic)
    elf_phdr* dyn_seg = NULL;
    for (int i = 0; i < hdr->e_phnum; i++) {
        if (phdrs[i].p_type == PT_DYNAMIC) {
            dyn_seg = &phdrs[i];
            break;
        }
    }
    
    if (!dyn_seg) {
        return -1;
    }
    
    Elf64_Sym* symtab = NULL;
    char* strtab = NULL;
    
    uint64_t* dynamic = (uint64_t*)((char*)base_addr + dyn_seg->p_vaddr);
    if (!dynamic) {
        return -1;
    }
    
    int i = 0;
    while (dynamic[i] != DT_NULL) {
        uint64_t tag = dynamic[i++];
        uint64_t val = dynamic[i++];
        
        if (tag == DT_SYMTAB) {
            symtab = (Elf64_Sym*)((char*)base_addr + val);
        }
        else if (tag == DT_STRTAB) {
            strtab = (char*)base_addr + val;
        }
    }
    
    if (!symtab || !strtab) {
        return -1;
    }
    
    for (i = 0; i < 100; i++) {
        Elf64_Sym sym = symtab[i];
        
        if (sym.st_name == 0) {
            continue;
        }
        
        char* sym_name = strtab + sym.st_name;
        if (strcmp(sym_name, name) == 0) {
            if (sym.st_value == 0) {
                return -1;
            }
            
            *symbol_addr = (void*)((char*)base_addr + sym.st_value);
            return 0;
        }
    }
    
    return -1;
}
