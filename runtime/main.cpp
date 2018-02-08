#include "mpi.h"

#include "utils.h"
#include "Runtime.h"
#include "Worker.h"

int main(){
	MPI_Init(nullptr, nullptr);

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	printf("rank %d has started\n", world_rank);

	if (world_rank == 0){
		RuntimeNode runtime;
		runtime.setup();
		runtime.event_loop();
		printf("runtime %d main finished\n", world_rank);
	} else {
		WorkerNode worker;
		worker.setup();
		worker.event_loop();
		printf("worker %d main finished\n", world_rank);
	}

	MPI_Finalize();
}
