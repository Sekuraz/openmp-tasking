//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_WORKER_H
#define LIBTDOMP_WORKER_H

#include <map>
#include <mutex>

#include "utils.h"


class Worker {
public:
    explicit Worker(int node_id);

    void submit_task(Task* task);
    void handle_run_task(int* data, int length);

    void ** request_memory(int origin, int task_id);
    void handle_request_memory(int *data, int length);

    void setup();

private:
    std::map<int, STask> created_tasks;
    std::mutex modify_state;

    int node_id;
    int runtime_node_id;
    int capacity = 4;
};


#endif //LIBTDOMP_WORKER_H
