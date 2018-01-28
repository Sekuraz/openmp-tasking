#include "functions.h"
#include "roles.h"
#include "utils.h"

#include <cstdio>
#include "mpi.h"
#include <chrono>

#include <set>
#include <vector>
#include <deque>

using namespace std;
void re::event_loop(){
	using namespace std;
	printf("re event loop started\n");
	
	//status variables:
	int free_workers = 0;
	vector<worker_s> workers;
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	for(int i = 2; i < world_size; i++){
		worker_s w;
		w.node_id = i;
		w.capacity = 4;
		free_workers += w.capacity;
		workers.push_back(w);
	}	
	for(auto it = workers.begin(); it != workers.end(); it++){
		printf("Worker %d has capacity %d\n", it->node_id, it->capacity);
	}

	printf("total capacity = %d\n", free_workers);

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

		int* recv_buffer = new int[buffer_size];
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
				printf("re received a finished_task %d from worker %d\n", recv_buffer[0], source);
				free_workers++;
				for(auto it = workers.begin(); it != workers.end(); it++){
					if(it->node_id == source){
						it->capacity+=1;
						break;
					}
				}
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

		delete[](recv_buffer);

		while(free_workers > 0 && queued_tasks.size() > 0){
			int task_id = task_id_counter;
			task_id_counter++;

			int task_descr = queued_tasks.front();
			queued_tasks.pop_front();
			for(auto it = workers.begin(); it != workers.end(); it++){
				if(it->capacity > 0){
					it->capacity--;
					printf("re scheduling new task with id %d on worker %d, remaining capacity = %d \n",
						 	task_id, it->node_id, it->capacity);
					run_task(task_id, &task_descr, 1, it->node_id);
					break;
				}	
			}
			
			running_tasks.insert(task_id);
			free_workers--;
		}

		//check for termination:
		printf("running_tasks = %d, queued_tasks = %d, main_waiting = %d\n",
				running_tasks.size(), queued_tasks.size(), main_waiting);
		if(running_tasks.size() == 0
				&& queued_tasks.size() == 0
				&& main_waiting){
			printf("re termination requirements fulfilled\n");
			for(worker_s w : workers){
				printf("re sending quit to worker %d\n", w.node_id);
				quit(w.node_id);
			}
			printf("re sending quit to main thread\n");
			quit(main_rank);
			quit_loop = true;
		}
	}
	printf("re event loop finished\n");
}
