#include "../include/mylib.h"

// Définition des fonctions
const char* foo_exported() {
    return "Hello from foo_exported()";
}

const char* bar_exported() {
    return "Hello from bar_exported()";
}

// Fonction cachée (sera hidden avec -fvisibility=hidden)
const char* hidden_func() {
    return "Hello from hidden_func()";
}

// Fonctions qui importent depuis le loader
const char* bar_imported(){
    // Force use of hidden_func to prevent optimization
    const char* temp = hidden_func(); // Use it to prevent it from being optimized out
    return new_bar();
}

const char* foo_imported(){
    return new_foo();
}