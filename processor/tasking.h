//
// Created by markus on 1/11/18.
//

#ifndef PROCESSOR_TASKING_H
#define PROCESSOR_TASKING_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <malloc.h>
#include <sys/resource.h>

#include "/tmp/tasking_functions/all.hpp"


void setup_tasking() {};
void teardown_tasking() {};

void taskwait() {};

// get the number of bytes which should be transmitted to the other node for the given pointer
size_t get_allocated_size(void* pointer) {
    auto t = (size_t) pointer;

    std::ifstream maps("/proc/self/maps");
    bool isHeap = false;
    std::string heap("heap"), stack("stack");

    size_t start_of_heap = 0;

    for (std::string line; std::getline(maps, line); ) {
        size_t start, end;

        // this char reads the '-' between the 2 values in the file, otherwise the return value is incorrect
        char discard;

        if (line.find(heap) != std::string::npos) {

            std::istringstream iss(line);
            iss >> std::hex >> start >> discard >> end;


            if (start < t && t < end) {
                isHeap = true;
                start_of_heap = start;
                break;
            }
        }
        if (line.find(stack) != std::string::npos) {
            std::istringstream iss(line);

            iss >> std::hex >> start >> discard >> end;

            if (start < t && t < end) {
                // return the maximum stack size, this should be low enough to not be that much of a burden
                // TODO look wether the relevant memory is already transmitted
                rlimit lim;
                getrlimit(RLIMIT_STACK, &lim);
                return lim.rlim_cur;
            }
        }
    }

    if (!isHeap) {
        throw std::domain_error("unable to determine the size of a pointee");
    }

    size_t loops = 0;
    size_t size = 0;
    do {
        size = malloc_usable_size((void*)t);
        t--;
        loops++;
        if ((size_t) pointer <= start_of_heap) {
            throw std::domain_error("unable to determine the size of a pointee");
        }
    } while (size == 0);

    return size - loops;
}

enum access_type {at_shared, at_firstprivate, at_private, at_default};

struct Var {
    std::string name;
    void * pointer;
    access_type access;
    size_t size;
};

class Task {
public:
    Task(unsigned long long code_id)
            : code_id(code_id), if_clause(true), final(false), untied(false), shared_by_default(true), mergeable(true),
              priority(0)
    {}

    void prepare() {
        for (auto& var : vars) {
            if (var.size == 0) {
                var.size = get_allocated_size(var.pointer);
            }
        }
    }

    void schedule() {
        this->prepare();
        void * arguments[this->vars.size()];
        for (int i = 0; i < vars.size(); i++) {
            arguments[i] = vars[i].pointer;
        }
        tasking_function_map[code_id](arguments);
    }

    unsigned long long code_id;

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
