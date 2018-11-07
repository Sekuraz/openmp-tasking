//
// Created by markus on 05.11.18.
//

#ifndef PROJECT_RUNTIME_H
#define PROJECT_RUNTIME_H

#include <map>
#include <mpi.h>
#include <vector>
#include <memory>

#include "utils.h"
#include "Scheduler.h"
#include "Receiver.h"

class Runtime : public Receiver {
public:
    explicit Runtime(int node_id, int world_size);

    void run_task_on_node(STask task, int node_id);

    void setup();

    Scheduler scheduler;

    void handle_message();

    void handle_create_task(int * data);



private:
    int world_size;
    int next_task_id = 0;


    std::vector<RuntimeWorker> workers;


};


#endif //PROJECT_RUNTIME_H
