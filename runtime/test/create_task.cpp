//
// Created by markus on 05.11.18.
//

#include <mpi.h>
#include <thread>
#include <chrono>

#include "../src/globals.h"
#include "../src/Task.h"
#include "../src/Runtime.h"
#include "../src/Worker.h"
#include "../src/constants.h"

using namespace std;

int main(int argc, char ** argv) {
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == 0) {
        Runtime r;
        while (r.created_tasks.size() < world_size - 1) {
            r.receive_message();
        }

        if (world_size > 2 && r.created_tasks[0]->priority != 42) {
            cout << "wrong priority" << endl;
            exit(EXIT_FAILURE);
        }

        if (world_size > 3 && r.created_tasks[1]->parent_id != -1) {
            cout << "wrong parent id" << endl;
            exit(EXIT_FAILURE);
        }
    }
    else {
        Worker w(world_rank);
        current_task = new Task(0);
        current_task->worker = &w;

        Task t(1);
        t.priority = 42;
        t.schedule();
    }

    MPI_Finalize();
}