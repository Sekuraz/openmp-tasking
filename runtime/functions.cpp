#include "utils.h"
#include "mpi.h"
#include "functions.h"
#include <algorithm>

using namespace std;

void create_task(int* task_descr, int length){
	MPI_Send(task_descr, length, MPI_INT, re_rank, TAG_CREATE, MPI_COMM_WORLD);	
}

void run_task(int task_id, int* task_descr, int length){
	int* message = (int*) malloc((length+1)*sizeof(int));
	message[0] = task_id;
	copy(task_descr, task_descr + length, message+1);
	MPI_Send(message, length+1, MPI_INT, worker_rank, TAG_COMMAND, MPI_COMM_WORLD);
	free(message);
}

void finish_task(int task_id){
	MPI_Send(&task_id, 1, MPI_INT, re_rank, TAG_COMMAND, MPI_COMM_WORLD);
}

void quit(int target_rank){
	int message = 0;
	MPI_Send(&message, 1, MPI_INT, target_rank, TAG_QUIT, MPI_COMM_WORLD);
}

void wait_quit(){
	int message = 0;
	MPI_Recv(&message, 1, MPI_INT, re_rank, TAG_QUIT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void wait_all(){
	int message = 0;
	MPI_Send(&message, 1, MPI_INT, re_rank, TAG_WAIT, MPI_COMM_WORLD);
}
