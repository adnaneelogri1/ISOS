#ifndef MYLIB_H
#define MYLIB_H

// Fonctions exportées (visibles par défaut)
const char* foo_exported();
const char* bar_exported();

// Cette fonction est normalement cachée en mode -fvisibility=hidden
// mais visible par défaut sans cette option
const char* hidden_func();

// Fonctions qui importent depuis le loader
const char* bar_imported(); 
const char* foo_imported();

// Fonctions importées du loader
extern const char* new_foo();
extern const char* new_bar();


//chall7 
// Exported symbol table
extern const char* imported_symbols[];

#endif