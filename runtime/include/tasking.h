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

class Task;
void run_runtime(int);



void setup_tasking(int arg_c, char** arg_v) {
    MPI_Init(nullptr, nullptr);

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	if (world_rank == 0){
        run_runtime(world_rank);
    } else {
	    argc = arg_c;
	    argv = arg_v;
		run_worker(world_rank);
	}

	MPI_Finalize();
    exit(EXIT_SUCCESS);
};

void taskwait() {
    throw "taskwait is not implemented yet.";
};





#endif //PROCESSOR_TASKING_H
