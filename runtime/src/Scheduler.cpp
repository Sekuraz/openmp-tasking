//
// Created by markus on 06.11.18.
//

#include "Scheduler.h"
#include "Task.h"

void Scheduler::enqueue(STask task) {
    created_tasks.emplace(task->task_id, task);
}

bool Scheduler::task_available() {
    return !ready_tasks.empty();
}

void Scheduler::add_worker(std::shared_ptr<RuntimeWorker> worker) {
    workers.emplace(worker->node_id, worker);
}

std::shared_ptr<RuntimeWorker> Scheduler::get_worker(int node_id) {
    return workers[node_id];
}
