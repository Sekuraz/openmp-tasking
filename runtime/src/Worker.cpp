//
// Created by markus on 05.11.18.
//

#include <mpi.h>
#include <iomanip>
#include <cstdint>

#include "constants.h"
#include "Worker.h"
#include "Task.h"

#include "globals.h"

using namespace std;
void __main__(int, char **);


void Worker::handle_create_task(STask task) {
    lock_guard lock(mpi_receive_lock);

    task->prepare();
    task->parent_id = current_task->task_id;
    task->origin_id = node_id;


    auto data = task->serialize();
    MPI_Send(&data[0], (int)data.size(), MPI_INT, this->runtime_node_id, TAG::CREATE_TASK, MPI_COMM_WORLD);

    int task_id;
    MPI_Recv(&task_id, 1, MPI_INT, this->runtime_node_id, TAG::CREATE_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    task->task_id = task_id;

    this->created_tasks.emplace(task_id, task);

    MPI_Send(nullptr, 0, MPI_INT, this->runtime_node_id, TAG::CREATE_TASK, MPI_COMM_WORLD);

    cout << setw(6) << node_id << ": created task " << task->task_id << ", origin: " << task->origin_id
         << ", runtime: " << this->runtime_node_id
         << ", task_id: " << task_id << endl;

}

Worker::Worker(int node_id) : Receiver(node_id), runtime_node_id((node_id / worker_per_runtime) * worker_per_runtime) {

}

void Worker::setup() {
    MPI_Send(&this->capacity, 1, MPI_INT, this->runtime_node_id, TAG::REPORT_CAPACITY, MPI_COMM_WORLD);
    should_run = true;
}

void Worker::handle_finish_task(STask task) {
    //lock_guard lock(mpi_receive_lock);

    task->finished = true;
    task->running = false;

    this->free_capacity += task->capacity;


    if (task->task_id != 0) {
        write_memory(task);
    }

    int buffer[] = {task->task_id, task->capacity};
    MPI_Send(buffer, 2, MPI_INT, this->runtime_node_id, TAG::FINISH_TASK, MPI_COMM_WORLD);
    running_tasks.erase(task->task_id);

    cout << setw(6) << node_id << ": Task " << task->task_id << " finished" << endl;
}

void main_entry(STask task) {
    current_task = task;
    __main__(argc, argv);
}

void task_entry(STask task, void (*func)(size_t **), size_t** arguments) {
    current_task = task;
    func(arguments);
}

void Worker::handle_run_task(STask task) {
    //lock_guard lock(mpi_receive_lock);

    running_tasks.emplace(task->task_id, task);

    // main task has id 0 and different invocation
    if (task->task_id != 0 && task->variables_count != 0) {
        if (task->origin_id == this->node_id) {
            created_tasks[task->task_id]->update(task);
            task = created_tasks[task->task_id];

            task->memory = (size_t **) malloc(task->variables_count * sizeof(size_t));
            for (int i = 0; i < task->vars.size(); i++) {
                task->memory[i] = (size_t*) task->vars[i].pointer;
            }
        }
        else {
            request_memory(task->origin_id, task);
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
        task->run_thread = new thread(task_entry, task, func, task->memory);
    }

    this->free_capacity -= 1;
    MPI_Send(&this->free_capacity, 1, MPI_INT, this->runtime_node_id, TAG::REPORT_CAPACITY, MPI_COMM_WORLD);
}

void Worker::handle_request_memory(int task_id, int source) {
    auto task = created_tasks[task_id];
    task->variables_count = task->vars.size();

    MPI_Send(&task->variables_count, 1, MPI_INT, source, TAG::REQUEST_MEMORY, MPI_COMM_WORLD);

    vector<int> sizes(task->variables_count);
    for (int i = 0; i < task->variables_count; i++) {
        sizes[i] = task->vars[i].size;
    }
    MPI_Send(&sizes[0], sizes.size(), MPI_INT, source, TAG::REQUEST_MEMORY, MPI_COMM_WORLD);

    for (int i = 0; i < task->variables_count; i++) {
        //cout << node_id << ": Sending: " << sizes[i] << " to " << source << endl;
        MPI_Send(task->vars[i].pointer, task->vars[i].size, MPI_BYTE, source, TAG::REQUEST_MEMORY, MPI_COMM_WORLD);
    }
}

void Worker::request_memory(int origin, STask task) {

    lock_guard lock(mpi_receive_lock); // locked by handle_run_task??

    MPI_Send(&task->task_id, 1, MPI_INT, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD);

    int variables_count = 0;
    MPI_Recv(&variables_count, 1, MPI_INT, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    //cout << node_id << ": Receiving: " << variables_count << " variables from " << origin << endl;

    vector<int> sizes(variables_count);
    MPI_Recv(&sizes[0], sizes.size(), MPI_INT, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    task->memory = (size_t **) malloc(variables_count * sizeof(size_t));

    MPI_Status status;

    for (int i = 0; i < variables_count; i++) {
        task->memory[i] = (size_t *)malloc(sizes[i]);

        //cout << node_id << ": Receiving: " << sizes[i] << " from " << origin << endl;

        MPI_Recv(task->memory[i], sizes[i], MPI_BYTE, origin, TAG::REQUEST_MEMORY, MPI_COMM_WORLD, &status);

        void * backup = malloc(sizes[i]);
        memcpy(backup, task->memory[i], sizes[i]);
        task->vars.emplace_back(Var("", backup, access_type::at_none, sizes[i], false));
    }

    //cout << "MEMORY TRANSFERRED" << endl;
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
            cout << setw(6) << node_id << ": Request to run task " << task->task_id << " free capacity: " << free_capacity << endl;
            handle_run_task(task);
            break;
        case TAG::SHUTDOWN:
            should_run = false; // There should be enough delay to let the thread finish
            break;
        case TAG::REQUEST_MEMORY:
            handle_request_memory(m.data[0], m.source); // There should be enough delay to let the thread finish
            break;
        case TAG::WRITE_MEMORY:
            handle_write_memory(m.data[0], m.source); // There should be enough delay to let the thread finish
            break;
        default:
            cout << setw(6) << node_id << ": Received unknown tag " << m.tag << " from " << m.source << endl;
    }
}

void Worker::write_memory(STask task) {
    cout << setw(6) << node_id << ": Writing back for " << task->task_id << " to " << task->origin_id << endl;


    //lock_guard mem(task->memory_lock); // prevent others from messing with the memory during this call

    // if we were running local -> no need for a transfer
    if (task->origin_id != node_id) {

        MPI_Send(&task->task_id, 1, MPI_INT, task->origin_id, TAG::WRITE_MEMORY, MPI_COMM_WORLD);

        for (size_t var = 0; var < task->variables_count; var++) {
            uint64_t change_count = 0;
            size_t end = 1 + ((task->vars[var].size - 1) / sizeof(uint8_t)); // size if pointer is integer (4 byte)

            auto backup = (uint8_t *) task->vars[var].pointer;
            auto data = (uint8_t *) task->memory[var];

            for (size_t i = 0; i < end; i ++) {
                change_count += (backup[i] != data[i]);
            }

            auto buffer = vector<size_t>();
            //buffer.reserve(change_count * 2); // we need that many elements

            for (size_t i = 0; i < end; i++) {
                if (backup[i] != data[i]) {
                    buffer.emplace_back(i);
                    buffer.emplace_back(data[i]);
                }
            }

//            for (int i = 0; i < change_count * 2; i += 2) {
//                cout << "=Task " << task->task_id << " changed value of " << var << " at " << buffer[i] << " from "
//                    << (int)backup[buffer[i]] << " to " << (int)buffer[i + 1] << endl;
//            }

            auto size = buffer.size();
            MPI_Send(&size, 1, MPI_UINT64_T, task->origin_id, TAG::WRITE_MEMORY, MPI_COMM_WORLD);
            MPI_Send(&buffer[0], size, MPI_UINT64_T, task->origin_id, TAG::WRITE_MEMORY, MPI_COMM_WORLD);
        }
    }

    // free the backup
    task->free_after_run(task->origin_id == node_id);
}

void Worker::handle_write_memory(int task_id, int source) {
    auto task = created_tasks[task_id];
    lock_guard rec(mpi_receive_lock);
    //lock_guard mem(task->memory_lock);

    cout << setw(6) << node_id << ": Receiving back for " << task->task_id << " from " << source << endl;


    for (auto& var : task->vars) {

        auto memory = (uint8_t *) var.pointer;

        uint64_t elems = 0;
        MPI_Recv(&elems, 1, MPI_UINT64_T, source, TAG::WRITE_MEMORY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        auto buffer = vector<uint64_t>(elems);
        MPI_Recv(&buffer[0], elems, MPI_UINT64_T, source, TAG::WRITE_MEMORY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (uint64_t elem = 0; elem < elems; elem += 2) {
            memory[buffer[elem]] = (uint8_t) buffer[elem + 1];
        }
    }
}
