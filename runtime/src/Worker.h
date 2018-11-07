//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_WORKER_H
#define LIBTDOMP_WORKER_H

#include <map>

#include "utils.h"
#include "Receiver.h"


class Worker : public Receiver {
public:
    explicit Worker(int node_id);

    // methods invoked locally
    void handle_create_task(STask task);
    void handle_finish_task();

    void handle_run_task(int* data, int length);

    void ** request_memory(int origin, int task_id);
    void handle_request_memory(int *data, int length);

    void setup();

private:
    std::map<int, STask> created_tasks;

    int runtime_node_id;
    int capacity = 4;
};


#endif //LIBTDOMP_WORKER_H
