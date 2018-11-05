//
// Created by markus on 05.11.18.
//

#include <mpi.h>

#include "constants.h"
#include "Worker.h"
#include "Task.h"


using namespace std;

void Worker::submit_task(Task *task) {
    task->origin_id = worker_id;

    lock_guard lock(this->modify_state);
    auto data = task->serialize();
    cout << "Creating new task for " << task->code_id << " on " << this->runtime_node_id << endl;
    MPI_Send(&data[0], data.size(), MPI_INT, this->runtime_node_id, TAG::CREATE_TASK, MPI_COMM_WORLD);

    int task_id;
    MPI_Recv(&task_id, 1, MPI_INT, this->runtime_node_id, TAG::CREATE_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    task->task_id = task_id;

    this->created_tasks.emplace(task_id, *task);

    cout << "Created new task for " << task->code_id << " on " << this->runtime_node_id << ", task_id is " << task_id << endl;

}

Worker::Worker(int worker_id) : worker_id(worker_id), runtime_node_id((worker_id / worker_per_runtime) * worker_per_runtime) {

}
