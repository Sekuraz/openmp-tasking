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
    MPI_Init(nullptr, nullptr);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == 0) {
        Runtime r(world_rank, world_size);
        while (r.scheduler.get_all_tasks().size() < world_size - 1) {
            r.handle_message();
        }

        if (world_size > 2 && r.scheduler.created_tasks[0]->priority != 42) {
            cout << "wrong priority" << endl;
            exit(EXIT_FAILURE);
        }

        if (world_size > 3 && r.scheduler.created_tasks[1]->parent_id != -1) {
            cout << "wrong parent id" << endl;
            exit(EXIT_FAILURE);
        }
    }
    else {
        auto w = make_shared<Worker>(world_rank);
        current_task = make_shared<Task>(0);
        current_task->worker = w;

        auto t = make_shared<Task>(1);
        t->priority = 42;
        current_task->worker.lock()->handle_create_task(t);
    }

    MPI_Finalize();
}