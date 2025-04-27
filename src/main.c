    /*#include <stdio.h>
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
    }*/

    #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <unistd.h>
#include "dynloader.h"
#include "debug.h"
#include "loader.h"

// Informations sur le programme
const char *argp_program_version = "isos_loader 1.0";
const char *argp_program_bug_address = "<adnane.elogri@univ-rennes1.fr>";

// Documentation
static char doc[] = "ISOS Loader - charge et exécute des fonctions depuis des bibliothèques partagées";
static char args_doc[] = "LIBRARY_PATH FUNCTION_NAME [FUNCTION_NAME...]";

// Options de ligne de commande
static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Afficher plus d'infos"},
    {"debug", 'd', "LEVEL", 0, "Définir niveau de debug (0-5)"},
    {0}
};

// Structure pour les arguments
struct arguments {
    char *lib_path;
    char **func_names;
    int func_count;
    int verbose;
    int debug_level;
};
// Fonctions exportées pour les bibliothèques
const char* new_bar() {
    return "Hello from new_bar()";
}

const char* new_foo() {
    return "Hello from new_foo()";
}

// Table des symboles exportés pour les bibliothèques
symbol_entry imported_functions[] = {
    {"new_foo", (void*)new_foo},
    {"new_bar", (void*)new_bar},
    {NULL, NULL}  // Fin de la table
};

// Traitement des options
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

// Parser argp
static struct argp argp = {options, parse_opt, args_doc, doc};

// Écriture sur stdout
void write_stdout(const char* str) {
    write(STDOUT_FILENO, str, strlen(str));
}


// Fonction principale
int main(int argc, char **argv) {
    struct arguments args;
    
    // Initialisation des arguments
    args.verbose = 0;
    args.func_count = 0;
    args.lib_path = NULL;
    args.func_names = NULL;
    args.debug_level = DBG_ERROR;
    
    // Parsing des arguments
    argp_parse(&argp, argc, argv, 0, 0, &args);
    
    // Initialisation du debug
    debug_init(args.debug_level);
    
    if (args.verbose) {
        debug_info("Debug activé");
        debug_info("Chargement de bibliothèque");
    }

    // Chargement de la bibliothèque
    void *handle = my_dlopen(args.lib_path);
    if (!handle) {
        debug_error("Échec du chargement");
        return 1;
    }
    
    // Configuration de la résolution PLT
    if (my_set_plt_resolve(handle, imported_functions) != 0) {
        debug_error("Échec configuration PLT resolver");
        return 1;
    }

    // Initialisation de la bibliothèque
    if (init_library(handle, imported_functions) != 0) {
        debug_warn("Échec initialisation bibliothèque");
        // On continue malgré tout, ce n'est pas forcément fatal
        exit(1);
    }

    // Exécution des fonctions demandées
    for (int i = 0; i < args.func_count; i++) {
        if (args.verbose) {
            debug_info("Recherche de fonction");
        }
        
        // Recherche de l'adresse de la fonction
        void* func_addr = my_dlsym(handle, args.func_names[i]);
        if (func_addr) {
            debug_info("Adresse de fonction trouvée");
            
            // Afficher l'adresse
            write_stdout(args.func_names[i]);
            write_stdout("() => Adresse: 0x");
            
            // Convertir l'adresse en hex
            char addr_str[20];
            sprintf(addr_str, "%lx", (unsigned long)func_addr);
            write_stdout(addr_str);
            write_stdout("\n");
            
            // Appel de la fonction
            const char* (*func_ptr)() = (const char* (*)())func_addr;
            const char* result = func_ptr();
            
            // Afficher le résultat
            write_stdout("Résultat de l'appel: ");
            write_stdout(result);
            write_stdout("\n");
        } else {
            debug_error("Fonction non trouvée");
            fprintf(stderr, "ERREUR: Fonction '%s' non trouvée dans la bibliothèque\n", args.func_names[i]);
        }
    }
    
    // Nettoyage
    if (args.func_names) {
        free(args.func_names);
    }
    
    debug_info("Fin du programme");
    
    return 0;
}

