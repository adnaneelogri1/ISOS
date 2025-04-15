#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <dlfcn.h> 
#include <unistd.h>
#include "../include/dynloader.h"
#include "../include/debug.h"

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

// Fonction pour écrire un résultat de fonction
void write_result(const char* func_name, const char* result) {
    write_stdout(func_name);
    write_stdout("() => ");
    if (result) {
        write_stdout(result);
    } else {
        write_stdout("(null)");
    }
    write_stdout("\n");
}

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
    
    for (int i = 0; i < args.func_count; i++) {
        if (args.verbose) {
            debug_info("Recherche de fonction");
        }
        
        void* func_addr = my_dlsym(handle, args.func_names[i]);
        
        if (func_addr) {
            debug_info("Adresse de fonction trouvée");
            
            // Ne pas essayer d'exécuter directement la fonction
            // Au lieu de ça, affichons simplement qu'on l'a trouvée
            write_stdout(args.func_names[i]);
            write_stdout("() => Adresse: 0x");
            
            // Convertir l'adresse en chaîne hexadécimale
            char addr_str[20];
            sprintf(addr_str, "%lx", (unsigned long)func_addr);
            write_stdout(addr_str);
            write_stdout("\n");
        } else {
            debug_error("Fonction non trouvée");
        }
    }
    
    if (args.func_names) {
        free(args.func_names);
    }
    
    debug_info("Fermeture de la bibliothèque");
    dlclose(handle);
    
    return 0;
}