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

using namespace std;

void Runtime::receive_message() {
    int tag, source;
    MPI_Status status;

    int receive_pending = false;
    int receive_finished = false;

    if (!receiving) {

        //printf("runtime %d recv is not running. probing...\n", node_id);
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &receive_pending, &status);
    }
    if (receive_pending) {
        int buffer_size;
        //printf("runtime %d started receiving\n", node_id);
        MPI_Get_count(&status, MPI_INT, &buffer_size);

        tag = status.MPI_TAG;
        source = status.MPI_SOURCE;

        cout << "Start receive " << buffer_size << " bytes from " << source << " with tag " << tag << endl;

        buffer = new int[buffer_size];
        MPI_Irecv(buffer, buffer_size, MPI_INT, source, tag, MPI_COMM_WORLD, &current_request);

        receiving = true;
    }
    if (receiving) {
        MPI_Test(&current_request, &receive_finished, &status);
        if (receive_finished) {
            receiving = false;
            tag = status.MPI_TAG;
            source = status.MPI_SOURCE;

            cout << "Received " << tag << " from " << source << endl;

            switch (tag) {
                case TAG::CREATE_TASK:
                    handle_create_task(buffer);
                    break;
                default:
                    cout << "Received unknown tag from " << source << ": " << tag << endl;
                    break;
            }
        }
    }
}

void Runtime::handle_create_task(int *data) {
    auto task = Task::deserialize(data);
    task->task_id = next_task_id++;
    scheduler.enqueue(task);

    // Return the task_id to the caller
    MPI_Send(&task->task_id, 1, MPI_INT, task->origin_id, TAG::CREATE_TASK, MPI_COMM_WORLD);
}

void Runtime::run_task_on_node(STask task, int node_id) {
    auto data = task->serialize();
    // Wait for completion, otherwise the buffer may be deallocated
    MPI_Send(&data[0], data.size(), MPI_INT, node_id, TAG::RUN_TASK, MPI_COMM_WORLD);
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

	scheduler.enqueue(root_task);
}

Runtime::Runtime(int node_id, int world_size) : node_id(node_id), world_size(world_size) {

}
