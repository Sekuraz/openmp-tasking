#include "utils.h"
#include "mpi.h"
#include "functions.h"
#include <algorithm>

using namespace std;

void create_task(int parent_id, int code_id){
	int message_size = 2;
	int* buffer = new int[message_size];
	buffer[0] = parent_id;
	buffer[1] = code_id;
	MPI_Send(buffer, message_size, MPI_INT, re_rank, TAG_CREATE, MPI_COMM_WORLD);	
	delete[](buffer);
}

void run_task(int task_id, int code_id,  int worker_id){
	int message_size = 2;
	int* message = new int[message_size];
	message[0] = task_id;
	message[1] = code_id;
	MPI_Send(message, message_size, MPI_INT, worker_id, TAG_COMMAND, MPI_COMM_WORLD);
	delete[](message);
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
