//
// Created by markus on 06.11.18.
//

#include "Scheduler.h"
#include "Task.h"

using namespace std;

void Scheduler::enqueue(STask task) {
    created_tasks.emplace(task->task_id, task);

    if (can_run_task(task)) {
        ready_tasks.push(task);
    }
}

bool Scheduler::work_available() {
    bool node_available = false;

    for (auto & worker : workers) {
        if (worker.second->free_capcaity > 0) {
            node_available = true;
            break;
        }
    }

    return node_available && !ready_tasks.empty();
}

void Scheduler::add_worker(std::shared_ptr<RuntimeWorker> worker) {
    workers.emplace(worker->node_id, worker);
}

std::shared_ptr<RuntimeWorker> Scheduler::get_worker(int node_id) {
    return workers[node_id];
}

std::pair<int, STask> Scheduler::get_next_node_and_task() {
    int run_node_id = 0, free_capacity = 0;

    for (auto& worker : workers) {
        if (worker.second->free_capcaity > free_capacity) {
            run_node_id = worker.first;
            free_capacity = worker.second->free_capcaity;
        }
    }

    STask to_run;
    while (!ready_tasks.empty()) {
        to_run = ready_tasks.front();
        ready_tasks.pop();
        if (!can_run_task(to_run)) {
            continue;
        }
        else {
            return make_pair(run_node_id, to_run);
        }
    }

    return make_pair(0, nullptr);
}

bool Scheduler::can_run_task(STask task) {
    return task->code_id == 0;
}
