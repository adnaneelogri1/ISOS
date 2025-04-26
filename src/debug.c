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

// Fonction utilitaire pour écrire directement sur STDERR_FILENO
void write_stderr(const char* prefix, const char* msg) {
    // Écrire le préfixe
    write(STDERR_FILENO, prefix, strlen(prefix));
    
    // Écrire le message
    write(STDERR_FILENO, msg, strlen(msg));
    
    // Écrire un saut de ligne
    write(STDERR_FILENO, "\n", 1);
}

// Version avancée avec support de format
void debug_printf(int level, const char* format, ...) {
    if (debug_level < level) {
        return;
    }
    
    // Préfixe selon le niveau
    const char* prefix;
    switch (level) {
        case DBG_ERROR:   prefix = "[ERROR] "; break;
        case DBG_WARN:    prefix = "[WARN] "; break;
        case DBG_INFO:    prefix = "[INFO] "; break;
        case DBG_DETAIL:  prefix = "[DETAIL] "; break;
        case DBG_VERBOSE: prefix = "[VERBOSE] "; break;
        default:          prefix = "[LOG] "; break;
    }
    
    char buffer[1024]; // Taille raisonnable pour la plupart des messages
    va_list args;
    va_start(args, format);
    
    #if defined(__STDC_LIB_EXT1__) && __STDC_WANT_LIB_EXT1__ == 1
    // Utiliser vsnprintf_s si disponible (C11)
    vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, format, args);
    #else
    // Utiliser vsnprintf sinon, et s'assurer que la chaîne est terminée
    int result = vsnprintf(buffer, sizeof(buffer), format, args);
    if (result < 0 || (size_t)result >= sizeof(buffer)) {
        // En cas d'erreur ou de troncature, s'assurer que la chaîne est terminée
        buffer[sizeof(buffer) - 1] = '\0';
    }
    #endif
    va_end(args);
    
    // Écrire le message
    write_stderr(prefix, buffer);
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