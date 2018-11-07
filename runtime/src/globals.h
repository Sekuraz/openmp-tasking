//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_GLOBALS_H
#define LIBTDOMP_GLOBALS_H

#include <map>

class Task;

thread_local Task* current_task;

std::map<int, void (*)(void **)> tasking_function_map;

int argc;
char** argv;


#endif //LIBTDOMP_GLOBALS_H