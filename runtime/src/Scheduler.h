//
// Created by markus on 06.11.18.
//

#ifndef PROJECT_SCHEDULER_H
#define PROJECT_SCHEDULER_H

#include <map>
#include <memory>
#include <vector>
#include <queue>

#include "utils.h"

class Task;

class Scheduler {
public:
    void add_worker(std::shared_ptr<RuntimeWorker> worker);
    std::shared_ptr<RuntimeWorker> get_worker(int node_id);
    int get_worker_count() {return workers.size();};


    void enqueue(STask task);
    bool work_available();

    std::pair<int, STask> get_next_node_and_task();

    std::map<int, STask> get_all_tasks() {return created_tasks;}; // TODO remove

    std::map<int, STask> created_tasks;
    std::map<int, STask> running_tasks;
    std::queue<STask> ready_tasks;

private:
    std::map<int, std::shared_ptr<RuntimeWorker> > workers;

    bool can_run_task(STask task);

};


#endif //PROJECT_SCHEDULER_H
