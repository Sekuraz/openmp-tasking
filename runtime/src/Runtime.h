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


class Runtime {
public:
    explicit Runtime(int node_id, int world_size);

    void run_task_on_node(STask task, int node_id);
    void receive_message();

    void setup();

    Scheduler scheduler;


private:
    int node_id;
    int world_size;
    int next_task_id = 0;

    int receiving = false;
    int *buffer;
    MPI_Request current_request;
    std::vector<RuntimeWorker> workers;

    void handle_create_task(int * data);

};


#endif //PROJECT_RUNTIME_H
