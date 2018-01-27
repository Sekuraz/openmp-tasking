#include "mpi.h"
#include <cstdio>

#include "utils.h"
#include "functions.h"
#include "roles.h"

int main(){
	MPI_Init(nullptr, nullptr);

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	if(world_rank == main_rank){
			printf("main started\n");
			main_thread::event_loop();
	} else if(world_rank == re_rank) {
			printf("re started\n");
			re::event_loop();
	} else {
			printf("worker %d started\n", world_rank);
			Worker w;
			w.event_loop();
			printf("worker %d finished\n", world_rank);
	}
	MPI_Finalize();
}
