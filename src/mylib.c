#include <stddef.h>
#include "mylib.h"
#include "isos-support.h"
#include "loader.h"

// Définir les variables globales pour le handle et le trampoline
void *loader_handle = NULL;
void *isos_trampoline = NULL;

// Prototypes des fonctions importées
const char *new_foo();

const char *new_bar();

// Implémentation des fonctions exportées
const char *foo_exported() {
    return "Hello from foo_exported()";
}

const char *bar_exported() {
    return "Hello from bar_exported()";
}

// Implémentation des fonctions qui importent
const char *foo_imported() {
    return new_foo();
}

const char *bar_imported() {
    return new_bar();
}

// Table des symboles importés
const char *imported_symbols[] = {
    "new_foo", // ID 0
    "new_bar", // ID 1
    NULL
};

// Table des symboles exportés
symbol_entry exported_symbols[] = {
    {"foo_exported", (void *) foo_exported},
    {"bar_exported", (void *) bar_exported},
    {"foo_imported", (void *) foo_imported},
    {"bar_imported", (void *) bar_imported},
    {NULL, NULL} // Fin de la table
};

// Structure loader_info
loader_info_t loader_info = {
    .exported_symbols = exported_symbols,
    .imported_symbols = imported_symbols,
    .loader_handle = &loader_handle,
    .isos_trampoline = &isos_trampoline
};

// Entrées PLT pour les fonctions importées
PLT_BEGIN
PLT_ENTRY(0, new_foo)
PLT_ENTRY(1, new_bar)

// Fonction get_symbol_table pour l'entry point
symbol_entry *get_symbol_table() {
    return exported_symbols;
}
