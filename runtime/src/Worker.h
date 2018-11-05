//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_WORKER_H
#define LIBTDOMP_WORKER_H

#include <map>
#include <mutex>


class Task;

class Worker {
public:
    explicit Worker(int worker_id);

    void submit_task(Task* task);

private:
    std::map<int, Task> created_tasks;
    std::mutex modify_state;

    int worker_id;
    int runtime_node_id;
    int capacity = 4;
};


#endif //LIBTDOMP_WORKER_H
