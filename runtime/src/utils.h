//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_UTIL_H
#define LIBTDOMP_UTIL_H

#include <string>
#include <vector>
#include <memory>


class Task;
typedef std::shared_ptr<Task> STask;

enum access_type {at_shared, at_firstprivate, at_private, at_default};

struct Var {
    std::string name;
    void * pointer;
    access_type access;
    size_t size;
};

struct RuntimeWorker {
    std::vector<STask> running_tasks;
    int node_id;

    int capacity;
    int free_capcaity;
};


#endif //LIBTDOMP_UTIL_H
