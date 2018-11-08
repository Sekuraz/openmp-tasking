//
// Created by markus on 05.11.18.
//

#include <mpi.h>
#include <iomanip>

#include "constants.h"
#include "Worker.h"
#include "Task.h"

#include "globals.h"

using namespace std;
void __main__(int, char **);


void Worker::handle_create_task(STask task) {

    task->prepare();
    task->parent_id = current_task->task_id;
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

Worker::Worker(int node_id) : Receiver(node_id), runtime_node_id((node_id / worker_per_runtime) * worker_per_runtime) {

}

void Worker::setup() {
    MPI_Send(&this->capacity, 1, MPI_INT, this->runtime_node_id, TAG::REPORT_CAPACITY, MPI_COMM_WORLD);
    should_run = true;
}

void Worker::handle_finish_task(STask task) {
    task->finished = true;
    task->running = false;
    int buffer[] = {task->task_id, task->capacity};
    MPI_Send(buffer, 2, MPI_INT, this->runtime_node_id, TAG::FINISH_TASK, MPI_COMM_WORLD);
    running_tasks.erase(task->task_id);
}

void main_entry(STask task) {
    current_task = task;
    __main__(argc, argv);
}

void task_entry(STask task, void (*func)(void **), void** arguments) {
    current_task = task;
    func(arguments);
}

void Worker::handle_run_task(STask task) {
    running_tasks.emplace(task->task_id, task);

    void ** memory;

    if (task->task_id != 0 && task->variables_count != 0) {
        if (task->origin_id == this->node_id) {
            created_tasks[task->task_id]->update(task);
            task = created_tasks[task->task_id];

            memory = (void **) malloc(task->variables_count * sizeof(size_t));
            for (int i = 0; i < task->variables_count; i++) {
                memory[i] = task->vars[i].pointer;
            }
        }
        else {
            memory = request_memory(task->origin_id, task->task_id);
        }
    }

    task->worker = this;
    task->capacity = 1;
    task->running = true;

    if (task->task_id == 0) {
        task->run_thread = new thread(main_entry, task);
    }
    else {
        auto func = tasking_function_map[task->code_id];
        task->run_thread = new thread(task_entry, task, func, memory);
    }

    int capacity_change = -1;
    MPI_Send(&capacity_change, 1, MPI_INT, this->runtime_node_id, TAG::REPORT_CAPACITY, MPI_COMM_WORLD);
}

void Worker::handle_request_memory(int *data, int length) {
    auto task = created_tasks[data[0]];
}

void ** Worker::request_memory(int origin, int task_id) {
    lock_guard lock(mpi_receive_lock);
    MPI_Send(&task_id, 1, MPI_INT, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD);

    int variables_count = 0;
    MPI_Recv(&variables_count, 1, MPI_INT, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);


    throw "not implemented";
}

void Worker::run() {
    while (should_run) {

        handle_message();

        this_thread::sleep_for(chrono::milliseconds(1));
    }
}

void Worker::handle_message() {
    auto m = receive_message();

    if (m.tag == TAG::NO_MESSAGE) {
        return;
    }

    STask task = nullptr;

    switch(m.tag) {
        case TAG::RUN_TASK:
            task = Task::deserialize(&m.data[0]);
            handle_run_task(task);
            break;
        case TAG::SHUTDOWN:
            should_run = false; // There should be enough delay to let the thread finish
            break;
        default:
            cout << setw(6) << node_id << ": Received unknown tag " << m.tag << " from " << m.source << endl;
    }
}
