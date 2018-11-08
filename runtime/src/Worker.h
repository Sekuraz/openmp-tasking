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
    void handle_finish_task(STask task);

    void handle_run_task(STask task);

    void ** request_memory(int origin, int task_id);
    void handle_request_memory(int *data, int length);

    void setup();
    void run();
    void handle_message();

private:
    std::map<int, STask> created_tasks;
    std::map<int, STask> running_tasks; // In order not to deallocate tasks before their thread ends

    int runtime_node_id;
    int capacity = 4;

    bool should_run = false;
};


#endif //LIBTDOMP_WORKER_H
