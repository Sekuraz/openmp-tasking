//
// Created by markus on 05.11.18.
//

#ifndef PROJECT_RUNTIME_H
#define PROJECT_RUNTIME_H

#include <map>
#include <mpi.h>

class Task;

class Runtime {
public:
    explicit Runtime() = default;

    void run_task_on_node(Task * task, int node_id);


public:
    std::map<int, Task*> created_tasks;
    std::map<int, Task*> running_tasks;
    std::map<int, Task*> ready_tasks;

    int next_task_id = 0;

    int receiving = false;
    int *buffer;
    MPI_Request current_request;

    void receive_message();
    void handle_create_task(int * data);

};


#endif //PROJECT_RUNTIME_H
