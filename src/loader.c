#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <argp.h>
#include <dlfcn.h> 
#include "../include/dynloader.h"

const char *argp_program_version = "isos_loader 1.0";
const char *argp_program_bug_address = "<adnane.elogri@univ-rennes1.fr>";

static char doc[] = "ISOS Loader - loads and runs functions from shared libraries";

static char args_doc[] = "LIBRARY_PATH FUNCTION_NAME [FUNCTION_NAME...]";

static struct argp_option options[] = {
    {"verbose", 'v', 0, 0, "Print more info"},
    {0}
};

struct arguments {
    char *lib_path;
    char **func_names;
    int func_count;
    int verbose;
};

// parse options
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *args = state->input;

    switch (key) {
        case 'v':
            args->verbose = 1;
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

int main(int argc, char **argv) {
    struct arguments args;
    
    args.verbose = 0;
    args.func_count = 0;
    args.lib_path = NULL;
    args.func_names = NULL;
    
    argp_parse(&argp, argc, argv, 0, 0, &args);
    
    if (args.verbose) {
        printf("Loading: %s\n", args.lib_path);
    }
    
    void *handle = my_dlopen(args.lib_path);
    if (!handle) {
        return 1;
    }
    
    for (int i = 0; i < args.func_count; i++) {
        if (args.verbose) {
            printf("Looking up: %s\n", args.func_names[i]);
        }
        
        typedef const char* (*func_ptr)();
        func_ptr func = (func_ptr)my_dlsym(handle, args.func_names[i]);
        
        if (func) {
            const char *result = func();
            printf("%s() => %s\n", args.func_names[i], result);
        } else {
            fprintf(stderr, "Error: function %s not found\n", args.func_names[i]);
        }
    }
    
    if (args.func_names) {
        free(args.func_names);
    }
    
    dlclose(handle);
    
    return 0;
}
