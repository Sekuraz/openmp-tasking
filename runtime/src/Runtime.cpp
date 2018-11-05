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
    created_tasks.emplace(task->task_id, task);
    MPI_Send(&task->task_id, 1, MPI_INT, task->origin_id, TAG::CREATE_TASK, MPI_COMM_WORLD);
}
