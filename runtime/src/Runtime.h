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

    Scheduler scheduler;

    explicit Runtime(int node_id, int world_size);

    void setup();
    void run();
    void handle_message();

    void handle_create_task(STask task);
    void handle_finish_task(int task_id, int used_capacity, int source);

    void run_task_on_node(STask task, int node_id);
    void shutdown();


private:
    int world_size;
    int next_task_id = 0;

};


#endif //PROJECT_RUNTIME_H
