//
// Created by markus on 06.11.18.
//

#ifndef PROJECT_SCHEDULER_H
#define PROJECT_SCHEDULER_H

#include <map>
#include <memory>
#include <vector>
#include <list>

#include "utils.h"

class Task;

class Scheduler {
public:
    void add_worker(std::shared_ptr<RuntimeWorker> worker);

    void enqueue(STask task);
    void set_finished(STask task);
    bool work_available();

    STask first_unfinished_child(int task_id);

    std::pair<int, STask> get_next_node_and_task();

    std::map<int, STask> get_all_tasks() {return created_tasks;}; // TODO remove

    std::map<int, std::shared_ptr<RuntimeWorker> > workers;


    std::map<int, STask> created_tasks;
    std::map<int, STask> running_tasks;
    std::list<STask> ready_tasks;

private:

    bool can_run_task(STask task);

};


#endif //PROJECT_SCHEDULER_H
