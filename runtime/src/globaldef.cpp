//
// Created by markus on 07.11.18.
//

#ifndef PROJECT_GLOBALDEF_H
#define PROJECT_GLOBALDEF_H

#include <map>

#include "utils.h"

thread_local STask current_task;

std::map<int, void (*)(size_t**)> tasking_function_map;

int argc;
char** argv;


#endif //PROJECT_GLOBALDEF_H
