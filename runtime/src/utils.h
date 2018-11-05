//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_UTIL_H
#define LIBTDOMP_UTIL_H

#include <string>


enum access_type {at_shared, at_firstprivate, at_private, at_default};

struct Var {
    std::string name;
    void * pointer;
    access_type access;
    size_t size;
};


#endif //LIBTDOMP_UTIL_H
