#include "mpi.h"
#include <cstdio>

#include "utils.h"
#include "functions.h"
#include "roles.h"

int main(){
	MPI_Init(NULL, NULL);

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);


	switch(world_rank){

		case main_rank:
			printf("main started\n");
			main_thread::event_loop();
			break;
		case re_rank:
			printf("re started\n");
			re::event_loop();
			break;
		case worker_rank:
			printf("worker started\n");
			worker::event_loop();
			printf("worker finished\n");
			break;
		default:
			break;
	}

	MPI_Finalize();
}
