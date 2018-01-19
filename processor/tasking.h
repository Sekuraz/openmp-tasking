//
// Created by markus on 1/11/18.
//

#ifndef PROCESSOR_TASKING_H
#define PROCESSOR_TASKING_H

#include <vector>
#include <string>

enum access_type {at_shared, at_firstprivate, at_private, at_default};

struct Var {
    std::string name;
    void * pointer;
    access_type access;
};

class Task {
public:
    Task(int code_id)
            : code_id(code_id), if_clause(true), final(false), untied(false), shared_by_default(true), mergeable(true),
              priority(0)
    {}

    int code_id;

    bool if_clause; // if false, parent may not continue until this is finished
    bool final; // if true: sequential, also for children
    bool untied; // ignore, continue on same node, schedule on any
    bool shared_by_default; // ignore: syntax: default(shared | none) - are values shared by default or do they have to have a clause for it
    bool mergeable;// ignore
    std::vector<Var> vars; //all variables, in order
    //std::vector<void *> vars_private; //ignore: all variables by default, no write back necessary
    //std::vector<void *> firstprivate; //as above, but initialized with value before, no write back necessary
    //std::vector<void *> shared; //ignore, copy in/out, dev responsible for sync
    std::vector<std::string> in; //list of strings, runtime ordering for siblings, array sections?
    std::vector<std::string> out;
    std::vector<std::string> inout;
    int priority; //later

};

#endif //PROCESSOR_TASKING_H
