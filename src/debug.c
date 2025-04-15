#include "../include/debug.h"
#include <unistd.h>
#include <string.h>

// Niveau de debug global
int debug_level = DBG_ERROR;

// Initialiser le niveau de debug
void debug_init(int level) {
    debug_level = level;
}

// Fonction utilitaire pour écrire directement sur STDERR_FILENO
void write_stderr(const char* prefix, const char* msg) {
    // Écrire le préfixe
    write(STDERR_FILENO, prefix, strlen(prefix));
    
    // Écrire le message
    write(STDERR_FILENO, msg, strlen(msg));
    
    // Écrire un saut de ligne
    write(STDERR_FILENO, "\n", 1);
}

// Fonctions pour chaque niveau
void debug_error(const char* msg) {
    if (debug_level >= DBG_ERROR) {
        write_stderr("[ERROR] ", msg);
    }
}

void debug_warn(const char* msg) {
    if (debug_level >= DBG_WARN) {
        write_stderr("[WARN] ", msg);
    }
}

void debug_info(const char* msg) {
    if (debug_level >= DBG_INFO) {
        write_stderr("[INFO] ", msg);
    }
}

void debug_detail(const char* msg) {
    if (debug_level >= DBG_DETAIL) {
        write_stderr("[DETAIL] ", msg);
    }
}

void debug_verbose(const char* msg) {
    if (debug_level >= DBG_VERBOSE) {
        write_stderr("[VERBOSE] ", msg);
    }
}