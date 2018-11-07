//
// Created by markus on 05.11.18.
//


#include "Task.h"
#include "functions.h"
#include "globals.h"

using namespace std;

Task::Task(int code_id)
        : code_id(code_id),
          task_id(-1),
          origin_id(-1),
          if_clause(true),
          final(false),
          untied(false),
          shared_by_default(true),
          mergeable(true),
          priority(0)
{
    if (current_task) {
        parent_id = current_task->task_id;
    }
}

void Task::prepare() {
    for (auto& var : vars) {
        if (var.size == 0) {
            var.size = get_allocated_size(var.pointer);
        }
    }
}

void Task::schedule()  {
    this->prepare();
    void * arguments[this->vars.size()];
    for (int i = 0; i < vars.size(); i++) {
        arguments[i] = vars[i].pointer;
    }

    current_task->worker->submit_task(this);
}

vector<int> Task::serialize() {
    vector<int> output;
    output.emplace_back(code_id);
    output.emplace_back(task_id);
    output.emplace_back(origin_id);
    output.emplace_back(if_clause);
    output.emplace_back(final);
    output.emplace_back(untied);
    output.emplace_back(shared_by_default);
    output.emplace_back(mergeable);
    output.emplace_back(priority);
    output.emplace_back(parent_id);
    output.emplace_back(variables_count == -1 ? vars.size() : variables_count);

    return output;
}

STask Task::deserialize(int *input) {
    int index = 0;

    auto task = make_shared<Task>(input[index++]);
    task->task_id = input[index++];
    task->origin_id = input[index++];
    task->if_clause = input[index++] != 0;
    task->final = input[index++] != 0;
    task->untied = input[index++] != 0;
    task->shared_by_default = input[index++] != 0;
    task->mergeable = input[index++] != 0;
    task->priority = input[index++];
    task->parent_id = input[index++];
    task->variables_count = input[index++];

    return task;
}
