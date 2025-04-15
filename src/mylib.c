#include "../include/mylib.h"

const char* foo_exported() {
    return "Hello from foo_exported()";
}

const char* bar_exported() {
    return "Hello from bar_exported()";
}

const char* bar_imported(){
    return new_bar();
}

const char* foo_imported(){
    return new_foo();
}