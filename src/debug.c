#include "debug.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Niveau de debug global
int debug_level = DBG_ERROR;

// Initialiser le niveau de debug
void debug_init(int level) {
    debug_level = level;
}

// Fonction utilitaire pour écrire directement sur stdout
void write_stdout(const char *prefix, const char *msg) {
    // Écrire directement avec printf
    printf("%s%s\n", prefix, msg);
}

// Version avancée avec support de format
void debug_printf(int level, const char *format, ...) {
    if (debug_level < level) {
        return;
    }
    
    // Préfixe selon le niveau
    const char *prefix;
    switch (level) {
        case DBG_ERROR: prefix = "[ERROR] ";
            break;
        case DBG_WARN: prefix = "[WARN] ";
            break;
        case DBG_INFO: prefix = "[INFO] ";
            break;
        case DBG_DETAIL: prefix = "[DETAIL] ";
            break;
        case DBG_VERBOSE: prefix = "[VERBOSE] ";
            break;
        default: prefix = "[LOG] ";
            break;
    }
    
    // Utiliser printf directement
    printf("%s", prefix);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("\n");
}

// Fonctions pour chaque niveau
void debug_error(const char *msg) {
    if (debug_level >= DBG_ERROR) {
        write_stdout("[ERROR] ", msg);
    }
}

void debug_warn(const char *msg) {
    if (debug_level >= DBG_WARN) {
        write_stdout("[WARN] ", msg);
    }
}

void debug_info(const char *msg) {
    if (debug_level >= DBG_INFO) {
        write_stdout("[INFO] ", msg);
    }
}

void debug_detail(const char *msg) {
    if (debug_level >= DBG_DETAIL) {
        write_stdout("[DETAIL] ", msg);
    }
}

void debug_verbose(const char *msg) {
    if (debug_level >= DBG_VERBOSE) {
        write_stdout("[VERBOSE] ", msg);
    }
}