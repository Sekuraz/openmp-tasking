//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_TASK_H
#define LIBTDOMP_TASK_H

#include <vector>
#include <string>
#include <thread>

#include "utils.h"

class Worker;

class Task {
public:
    explicit Task(int code_id);

    void prepare();

    std::vector<int> serialize();

    static STask deserialize(int * input);

    int code_id;                // serialized
    int task_id;                // serialized
    int parent_id;              // serialized
    int origin_id;              // serialized
    bool finished;              // serialized
    bool running;
    int capacity;
    int variables_count = -1;   // serialized (and calculated if unknown)
    std::thread run_thread;
    std::weak_ptr<Worker> worker;


    bool if_clause; // if false, parent may not continue until this is finished
    bool final; // if true: sequential, also for children
    bool untied; // ignore, continue on same node, schedule on any
    bool shared_by_default; // ignore: syntax: default(shared | none) - are values shared by default or do they have to have a clause for it
    bool mergeable;// ignore
    std::vector<Var> vars; //all variables, in order
    std::vector<std::string> in; //list of strings, runtime ordering for siblings, array sections?
    std::vector<std::string> out;
    std::vector<std::string> inout;
    int priority; //later
};


#endif //LIBTDOMP_TASK_H
