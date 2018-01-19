#include "functions.h"
#include "roles.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include "mpi.h"

#include <deque>
#include <set>

using namespace std;

void worker::event_loop(){

	bool quit_loop = false;

	while(!quit_loop){
		printf("worker waiting for command\n");
		MPI_Status status;
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		int buffer_size;
		MPI_Get_count(&status, MPI_INT, &buffer_size);
		int tag = status.MPI_TAG;
		int source = status.MPI_SOURCE;

		int* recv_buffer = (int*) malloc(buffer_size*sizeof(int));
		MPI_Recv(recv_buffer, buffer_size, MPI_INT, 
				source, tag ,MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		switch(tag){
			case TAG_COMMAND:
				{
					printf("worker received COMMAND signal");
					int task_id = recv_buffer[0];
					int* task_descr = recv_buffer+1;
					int task_descr_length = buffer_size -1;

					printf("running task %d\n", task_id);
					//run the task here
					printf("finished task %d\n", task_id);

					finish_task(task_id);
				}
				break;
			case TAG_QUIT:
				printf("worker received QUIT signal\n");
				quit_loop = true;
				break;
			default:
				printf("worker received a message with the unknown tag: %d.\n", tag);
				break;
		}
		free(recv_buffer);

	}
	printf("worker event loop finished\n");
}

void main_thread::event_loop(){
	printf("main thread event loop started\n");

	//create some tasks here
	printf("main thread is creating a task\n");
	for(int i = 0; i < 8; i++){
		int task_descr = 1;
		create_task(&task_descr,1);
	}

	printf("main thread waiting for all tasks to finish\n");	
	wait_all();
	printf("main thread is waiting for finish signal\n");
	wait_quit();	
	printf("main thread event loop finished\n");
}

void re::event_loop(){
	using namespace std;
	printf("re event loop started\n");
	
	//status variables:
	int free_workers = 1;
  set<int> running_tasks;
	deque<int> queued_tasks;	
	int task_id_counter = 0;

	bool quit_loop = false;
	bool main_waiting = false;

	while(!quit_loop){
		MPI_Status status;
		MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

		int buffer_size;
		MPI_Get_count(&status, MPI_INT, &buffer_size);
		int tag = status.MPI_TAG;
		int source = status.MPI_SOURCE;

		int* recv_buffer = (int*) malloc(buffer_size*sizeof(int));
		MPI_Recv(recv_buffer, buffer_size, MPI_INT, 
				source, tag,MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		switch(tag){
			case TAG_WAIT:
				// main thread is waiting
				printf("re received wait_all from main thread\n");
				main_waiting = true;
				break;
			case TAG_COMMAND:
				// worker has finished a task
				printf("re received a finished_task from a worker\n");
				free_workers++;
				running_tasks.erase(recv_buffer[0]);
				break;
			case TAG_CREATE:
				// a new task is created
				printf("re received a new task from %d\n", source);
				queued_tasks.push_back(recv_buffer[0]);
				break;
			default:
				printf("re received a unknown tag: %d\n", tag);
				break;
		}

		free(recv_buffer);

		//check if there are free workers
		while(free_workers > 0 && queued_tasks.size() > 0){
			int task_id = task_id_counter;
			task_id_counter++;
			printf("re scheduling new task with id %d\n", task_id);

			int task_descr = queued_tasks.front();
			queued_tasks.pop_front();

			run_task(task_id, &task_descr, 1);
			running_tasks.insert(task_id);
			free_workers--;
		}

		//check for termination:
		if(running_tasks.size() == 0
				& queued_tasks.size() == 0
				& main_waiting){
			printf("re termination requirements fulfilled\n");
			printf("re sending quit to worker\n");
			quit(worker_rank);
			printf("re sending quit to main thread\n");
			quit(main_rank);
			quit_loop = true;
		}
	}
	printf("re event loop finished\n");
}
