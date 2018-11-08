//
// Created by markus on 05.11.18.
//

#define __MAIN__

#include "helper.h"

#include <mpi.h>

#include "../src/Runtime.h"
#include "../src/Worker.h"

using namespace std;

int main(int argc, char ** argv) {
    MPI_Init(nullptr, nullptr);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    if (world_rank == 0) {
        Runtime r(world_rank, world_size);
        r.setup();

        if (world_size - 1 != r.scheduler.workers.size()) {
            cout << "wrong number of workers" << endl;
            exit(EXIT_FAILURE);
        }

        for (int i = 1; i < world_size; i++) {
            if (r.scheduler.workers[i]->capacity != 4) {
                cout << "Worker " << i << " reported wrong capacity (set to 4 currently)" << endl;
                exit(EXIT_FAILURE);
            }
        }
        for (int i = 1; i < world_size; i++) {
            if (r.scheduler.workers[i]->free_capcaity != 4) {
                cout << "Worker " << i << " reported wrong free_capacity (set to 4 currently)" << endl;
                exit(EXIT_FAILURE);

            }
        }
    }
    else {
        auto w = make_shared<Worker>(world_rank);
        w->setup();
    }

    MPI_Finalize();
}