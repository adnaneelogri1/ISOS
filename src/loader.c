    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <argp.h>
    #include <unistd.h>
    #include <stddef.h>
    #include "dynloader.h"
    #include "debug.h"
    #include "loader.h"

    const char *argp_program_version = "isos_loader 1.0";
    const char *argp_program_bug_address = "<adnane.elogri@univ-rennes1.fr>";

    static char doc[] = "ISOS Loader - loads and runs functions from shared libraries";
    static char args_doc[] = "LIBRARY_PATH FUNCTION_NAME [FUNCTION_NAME...]";

    static struct argp_option options[] = {
        {"verbose", 'v', 0, 0, "Print more info"},
        {"debug", 'd', "LEVEL", 0, "Set debug level (0-5)"},
        {0}
    };

    struct arguments {
        char *lib_path;
        char **func_names;
        int func_count;
        int verbose;
        int debug_level;
    };

       
  // Notre table de symboles exportée
    symbol_entry imported_fonction[] = {
        {"new_foo", (void*)new_foo},
        {"new_bar", (void*)new_bar},
        {NULL, NULL} // Marque la fin de la table
    };



    static error_t parse_opt(int key, char *arg, struct argp_state *state) {
        struct arguments *args = state->input;

        switch (key) {
            case 'v':
                args->verbose = 1;
                break;
            case 'd':
                args->debug_level = atoi(arg);
                if (args->debug_level < 0) args->debug_level = 0;
                if (args->debug_level > 5) args->debug_level = 5;
                break;
            case ARGP_KEY_ARG:
                if (state->arg_num == 0) {
                    args->lib_path = arg;
                } else {
                    if (state->arg_num == 1) {
                        args->func_names = malloc(sizeof(char*) * (state->argc - 1));
                        if (!args->func_names) {
                            argp_usage(state);
                            return ENOMEM;
                        }
                    }
                    args->func_names[args->func_count++] = arg;
                }
                break;
            case ARGP_KEY_END:
                if (state->arg_num < 2) {
                    argp_usage(state);
                }
                break;
            default:
                return ARGP_ERR_UNKNOWN;
        }
        return 0;
    }

    static struct argp argp = {options, parse_opt, args_doc, doc};

    // Fonction utilitaire pour écrire sur stdout
    void write_stdout(const char* str) {
        write(STDOUT_FILENO, str, strlen(str));
    }

    // Fonctions exportées pour les imports
    const char* new_bar(){
        return "Hello from new_bar()";
    }
    const char* new_foo(){
        return "Hello from new_foo()";
    }

    int main(int argc, char **argv) {
        struct arguments args;
        
        args.verbose = 0;
        args.func_count = 0;
        args.lib_path = NULL;
        args.func_names = NULL;
        args.debug_level = DBG_ERROR;
        
        argp_parse(&argp, argc, argv, 0, 0, &args);
        
        // Initialiser le système de debug
        debug_init(args.debug_level);
        
        if (args.verbose) {
            debug_info("Debug activé");
            debug_info("Chargement de bibliothèque");
        }

        void *handle = my_dlopen(args.lib_path);
        if (!handle) {
            debug_error("Échec du chargement");
            return 1;
        }
            // Set up PLT resolution for the library
        if (my_set_plt_resolve(handle, imported_fonction) != 0) {
            debug_error("Failed to set PLT resolver");
            return 1;
        }

        void** loader_handle_tmp = loader_info.loader_handle;
        *loader_handle_tmp = handle;
        void** isos_trampoline_tmp = loader_info.isos_trampoline;
        *isos_trampoline_tmp = isos_trampoline;

        for (int i = 0; i < args.func_count; i++) {
            if (args.verbose) {
                debug_info("Recherche de fonction");
            }
            
            void* func_addr = my_dlsym(handle, args.func_names[i]);
            if (func_addr) {
            debug_info("Adresse de fonction trouvée");
            
            // Afficher l'adresse de la fonction
            write_stdout(args.func_names[i]);
            write_stdout("() => Adresse: 0x");
            
            // Convertir l'adresse en chaîne hexadécimale
            char addr_str[20];
            sprintf(addr_str, "%lx", (unsigned long)func_addr);
            write_stdout(addr_str);
            write_stdout("\n");
            
            // Cast du pointeur vers un type de fonction et appel
            const char* (*func_ptr)() = (const char* (*)())func_addr;
            const char* result = func_ptr();
            
            // Afficher le résultat de l'appel
            write_stdout("Résultat de l'appel: ");
            write_stdout(result);
            write_stdout("\n");
        }  else {
                // FIX: Afficher un message d'erreur approprié au lieu de segfault
                debug_error("Fonction non trouvée");
                fprintf(stderr, "ERROR: Function '%s' not found in library\n", args.func_names[i]);
            }
        }
        
        if (args.func_names) {
            free(args.func_names);
        }
        
        debug_info("Fermeture de la bibliothèque");
        
        return 0;
    }


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
    // Add this function to loader.c
void* loader_plt_resolver(void* handle, int sym_id) {
    if (!handle) {
        debug_error("Invalid handle in PLT resolver");
        return NULL;
    }
    
    lib_handle_t* lib_handle = (lib_handle_t*)handle;
    
    // Get imported symbols table from the library
    const char** imported_symbols = NULL;
    void* sym_addr = NULL;
    
    if (find_dynamic_symbol(lib_handle->base_addr, &lib_handle->hdr, 
                          lib_handle->phdrs, "imported_symbols", 
                          &sym_addr) != 0 || !sym_addr) {
        debug_error("Failed to find imported_symbols table");
        return NULL;
    }
    
    imported_symbols = (const char**)sym_addr;
    
    if (!imported_symbols || sym_id <= 0) {
        debug_error("Invalid symbol ID or imported symbols table");
        return NULL;
    }
    
    const char* sym_name = imported_symbols[sym_id];
    debug_info("Resolving PLT symbol");
    
    // Find symbol in our PLT resolve table
    symbol_entry* resolve_table = (symbol_entry*)lib_handle->plt_resolve_table;
    if (!resolve_table) {
        debug_error("PLT resolve table not set");
        return NULL;
    }
    
    // Search for the symbol
    int i = 0;
    while (resolve_table[i].name != NULL) {
        if (strcmp(resolve_table[i].name, sym_name) == 0) {
            debug_detail("Resolved PLT symbol");
            return resolve_table[i].addr;
        }
        i++;
    }
    
    debug_error("Symbol not found in PLT resolve table");
    return NULL;
}
int my_set_plt_resolve(void* handle, void* resolve_table) {
    if (!handle) {
        debug_error("Invalid handle");
        return -1;
    }
    lib_handle_t* lib_handle = (lib_handle_t*)handle;
    lib_handle->plt_resolve_table = resolve_table;
    return 0;
}