#ifndef DEBUG_H
#define DEBUG_H

// Niveaux de debug
#define DBG_NONE    0  // Pas de debug
#define DBG_ERROR   1  // Seulement les erreurs
#define DBG_WARN    2  // Erreurs + avertissements
#define DBG_INFO    3  // Info générale
#define DBG_DETAIL  4  // Informations détaillées
#define DBG_VERBOSE 5  // Très verbeux

// Niveau de debug par défaut
extern int debug_level;

// Initialiser le niveau de debug
void debug_init(int level);

// Fonction de debug avec format (comme printf)
void debug_printf(int level, const char* format, ...);

// Fonctions de log pour chaque niveau
void debug_error(const char* msg);
void debug_warn(const char* msg);  
void debug_info(const char* msg);
void debug_detail(const char* msg);
void debug_verbose(const char* msg);

#endif