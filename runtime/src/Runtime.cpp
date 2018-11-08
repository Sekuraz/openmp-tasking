//
// Created by markus on 05.11.18.
//

#include <mpi.h>
#include <memory>

#include "constants.h"
#include "Runtime.h"
#include "Task.h"


#include <thread>
#include <chrono>
#include <iomanip>

using namespace std;

Runtime::Runtime(int node_id, int world_size) : Receiver(node_id), world_size(world_size) {

}

void Runtime::handle_create_task(STask task) {
    task->task_id = next_task_id++;
    scheduler.enqueue(task);

    // Return the task_id to the caller
    MPI_Send(&task->task_id, 1, MPI_INT, task->origin_id, TAG::CREATE_TASK, MPI_COMM_WORLD);
}

void Runtime::handle_finish_task(int task_id, int used_capacity) {
    auto task = scheduler.created_tasks[task_id];
    task->finished = true;
    task->running = false;
    task->capacity = used_capacity;

}

void Runtime::run_task_on_node(STask task, int node_id) {
    auto data = task->serialize();
    // Wait for completion, otherwise the buffer may be deallocated
    MPI_Send(&data[0], (int)data.size(), MPI_INT, node_id, TAG::RUN_TASK, MPI_COMM_WORLD);
}

void Runtime::setup() {
    int world_size = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int own_id = (world_size / worker_per_runtime) * worker_per_runtime;
    int end = world_size >= own_id + worker_per_runtime ? own_id + worker_per_runtime : world_size;


	for (int i = own_id + 1; i < end; i++){
		auto w = make_shared<RuntimeWorker>();
		w->node_id = i;

		int capacity = 0;
        MPI_Recv(&capacity, 1, MPI_INT, i, TAG::REPORT_CAPACITY, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		w->capacity = capacity;
		w->free_capcaity = capacity;
		scheduler.add_worker(w);
	}

	auto root_task = make_shared<Task>(0);
	root_task->task_id = 0;

	scheduler.enqueue(root_task);
}

void Runtime::handle_message() {
    auto m = receive_message();


    if (m.tag == TAG::NO_MESSAGE) {
        return;
    }

    STask task = nullptr;

    switch(m.tag) {
        case TAG::CREATE_TASK:
            task = Task::deserialize(&m.data[0]);
            handle_create_task(task);
            break;
        case TAG::FINISH_TASK:
            handle_finish_task(m.data[0], m.data[1]);
            break;
        default:
            cout << setw(6) << node_id << ": Received unknown tag " << m.tag << " from " << m.source << endl;
    }
}

void Runtime::shutdown() {
    for (auto& [_, worker] : scheduler.workers) {
        MPI_Send(nullptr, 0, MPI_INT, worker->node_id, TAG::SHUTDOWN, MPI_COMM_WORLD);
    }
    MPI_Finalize();
    exit(EXIT_SUCCESS);
}


