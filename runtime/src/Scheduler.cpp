//
// Created by markus on 06.11.18.
//

#include "Scheduler.h"
#include "Task.h"

using namespace std;

void Scheduler::enqueue(STask task) {
    created_tasks.emplace(task->task_id, task);

    if (can_run_task(task)) {
        ready_tasks.push_back(task);
    }
}

bool Scheduler::work_available() {
    bool node_available = false;

    for (auto & worker : workers) {
        if (worker.second->free_capacity > 0) {
            node_available = true;
            break;
        }
    }

    return node_available && !ready_tasks.empty();
}

void Scheduler::add_worker(std::shared_ptr<RuntimeWorker> worker) {
    workers.emplace(worker->node_id, worker);
}

std::pair<int, STask> Scheduler::get_next_node_and_task() {
    int run_node_id = -1, free_capacity = 0;

    for (auto& [_, worker] : workers) {
        if (worker->free_capacity > free_capacity) {
            run_node_id = worker->node_id;
            free_capacity = worker->free_capacity;
        }
    }

    for (auto& task : ready_tasks) {
        if (!can_run_task(task)) {
            continue;
        }
        else {
            ready_tasks.remove(task);
            workers[run_node_id]->free_capacity--;
            return make_pair(run_node_id, task);
        }
    }

    return make_pair(0, nullptr);
}

bool Scheduler::can_run_task(STask task) {
    //return task->code_id == 0;
    return true; // no dependencies
}

void Scheduler::set_finished(STask task) {
    task->finished = true;
    task->running = false;

    task->children_finished = true;

    for (auto & child : task->children) {
        try {
            task->children_finished &= child.lock()->children_finished;
        }
        catch (bad_weak_ptr&) {
            // Children is already gone
        }
    }

    if (task->parent_id >= 0) {
        auto parent = created_tasks[task->parent_id];
        parent->children_finished = true;

        for (auto & child : parent->children) {
            try {
                parent->children_finished &= child.lock()->children_finished;
            }
            catch (bad_weak_ptr &) {
                // Children is already gone
            }
        }
    }

    // TODO the dependency stuff here
}

STask Scheduler::first_unfinished_child(int task_id) {
    auto task = created_tasks[task_id];
    for (auto& child : task->children) {
        if (!child.lock()->finished) {
            return child.lock();
        }
    }
}

