#ifndef MYLIB_H
#define MYLIB_H

const char* foo_exported();
const char* bar_exported();
const char* foo_imported();
const char* bar_imported(); 
extern const char* new_foo();
extern const char* new_bar();

#endif