//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_UTIL_H
#define LIBTDOMP_UTIL_H

#include <string>
#include <vector>
#include <memory>

#include "constants.h"

class Task;
typedef std::shared_ptr<Task> STask;

enum access_type {at_shared, at_firstprivate, at_private, at_default, at_none};

struct Var {
    Var(std::string name, void* pointer, access_type access, size_t size, bool copy)
        : name(std::move(name)), pointer(pointer), access(access), size(size), copy(copy) {}

    std::string name;
    void * pointer;
    access_type access;
    size_t size;
    bool copy;
};

struct RuntimeWorker {
    std::vector<STask> running_tasks;
    int node_id;

    int capacity;
    int free_capacity;
};

struct Message {
    TAG tag;
    int source;
    std::vector<int> data;
};

#endif //LIBTDOMP_UTIL_H
