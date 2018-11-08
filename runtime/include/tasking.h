//
// Created by markus on 1/11/18.
//

#ifndef PROCESSOR_TASKING_H
#define PROCESSOR_TASKING_H

#include <mpi.h>

#include "../src/globals.h"
#include "../src/Task.h"
#include "../src/Runtime.h"
#include "../src/Worker.h"

void do_tasking(int arg_c, char** arg_v) {
    MPI_Init(&arg_c, &arg_v);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_size < 2) {
        std::cout << "This code MUST be run by mpirun!" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (world_rank == 0) {
        auto r = std::make_shared<Runtime>(world_rank, world_size);
        r->setup();
        r->run();
    }
    else {
        auto w = std::make_shared<Worker>(world_rank);
        w->setup();
        w->run();
    }

    MPI_Finalize();
};

void taskwait() {
    throw "taskwait is not implemented yet.";
};





#endif //PROCESSOR_TASKING_H
