//
// Created by markus on 05.11.18.
//

#include <mpi.h>

#include "constants.h"
#include "Worker.h"
#include "Task.h"


using namespace std;
extern map<int, void (*)(void **)> tasking_function_map;

void Worker::submit_task(Task* task) {
    lock_guard lock(this->modify_state);

    task->origin_id = node_id;

    auto data = task->serialize();
//    cout << "Creating Task(code_id: " << task->code_id
//         << ", origin: " << task->origin_id
//         << ", runtime: " << this->runtime_node_id << ")" << endl;
    MPI_Send(&data[0], (int)data.size(), MPI_INT, this->runtime_node_id, TAG::CREATE_TASK, MPI_COMM_WORLD);

    int task_id;
    MPI_Recv(&task_id, 1, MPI_INT, this->runtime_node_id, TAG::CREATE_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    task->task_id = task_id;

    this->created_tasks.emplace(task_id, task);

    cout << "Created  Task(code_id: " << task->code_id
         << ", origin: " << task->origin_id
         << ", runtime: " << this->runtime_node_id
         << ", task_id: " << task_id << ")" << endl;
}

Worker::Worker(int node_id) : node_id(node_id), runtime_node_id((node_id / worker_per_runtime) * worker_per_runtime) {

}

void Worker::handle_run_task(int *data, int length) {
    auto task = Task::deserialize(data);

    void ** memory;

    if (task->task_id != 0 && task->variables_count != 0 && task->origin_id != this->node_id) {
        memory = request_memory(task->origin_id, task->task_id);
    }

    auto func = tasking_function_map[task->code_id];
    task->run_thread = thread(func, memory);
}

void Worker::handle_request_memory(int *data, int length) {
    auto task = created_tasks[data[0]];
}

void ** Worker::request_memory(int origin, int task_id) {
    MPI_Send(&task_id, 1, MPI_INT, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD);

    int variables_count = 0;
    MPI_Recv(&variables_count, 1, MPI_INT, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);


    throw "not implemented";
}

void Worker::setup() {
    MPI_Send(&this->capacity, 1, MPI_INT, this->runtime_node_id, TAG::REPORT_CAPACITY, MPI_COMM_WORLD);
}
