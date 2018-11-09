//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_GLOBALS_H
#define LIBTDOMP_GLOBALS_H

#include <map>

#include "utils.h"

extern thread_local STask current_task;

extern std::map<int, void (*)(size_t**)> tasking_function_map;

extern int argc;
extern char** argv;


#endif //LIBTDOMP_GLOBALS_H
