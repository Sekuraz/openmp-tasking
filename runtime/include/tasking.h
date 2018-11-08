//
// Created by markus on 1/11/18.
//

#ifndef PROCESSOR_TASKING_H
#define PROCESSOR_TASKING_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <malloc.h>
#include <mpi.h>
#include <sys/resource.h>
#include <map>

#include "../src/globals.h"
#include "../src/Task.h"
#include "../src/Runtime.h"
#include "../src/Worker.h"

void setup_tasking(int arg_c, char** arg_v) {
    MPI_Init(nullptr, nullptr);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    argc = arg_c;
    argv = arg_v;

    if (world_size < 2) {

    }

    if (world_rank == 0) {
        Runtime r(world_rank, world_size);
        r.setup();
    }
    else {
        Worker w(world_rank);
        w.setup();
    }

	MPI_Finalize();
    exit(EXIT_SUCCESS);
};

void taskwait() {
    throw "taskwait is not implemented yet.";
};





#endif //PROCESSOR_TASKING_H
