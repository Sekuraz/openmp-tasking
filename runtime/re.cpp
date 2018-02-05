#include "functions.h"
#include "roles.h"
#include "utils.h"
#include "waittree.h"

#include <cstdio>
#include "mpi.h"
#include <chrono>
#include <memory>

#include <set>
#include <vector>
#include <deque>
#include <unordered_map>

using namespace std;
void re::event_loop(){
	printf("re event loop started\n");

	set<int> running_tasks;
	deque<int> queued_tasks;	
	int id_counter = 0;

	//TODO
	//waittree hashmap here
	unordered_map<int, shared_ptr<WaitTree>> waittree_map;
	waittree_map.insert({id_counter, make_shared<WaitTree>()});
	int root_id = id_counter;
	id_counter++;

	
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

	printf("re: total capacity = %d\n", free_workers);

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
				printf("re: received wait_all from main thread\n");
				main_waiting = true;
				break;
			case TAG_COMMAND:
				// worker has finished a task
				{
					int task_id = recv_buffer[0];
					printf("re: received a finished_task %d from worker %d\n", recv_buffer[0], source);
					if(task_id != 0){
						free_workers++;
					}
					for(auto it = workers.begin(); it != workers.end(); it++){
						if(it->node_id == source){
							it->capacity+=1;
							break;
						}
					}
					running_tasks.erase(recv_buffer[0]);
					auto task_object = waittree_map[task_id];
					if(task_object == nullptr){
						printf("nullptr!\n");
					}
					task_object->finished_execution();
				}
				break;
			case TAG_CREATE:
				// a new task is created
				printf("re:  received a new task from task %d on worker %d\n", recv_buffer[0], source);
				{
					int parent_id = recv_buffer[0];
					int code_id = recv_buffer[1];
					int task_id = id_counter++;
					//put the code id in the queue
					queued_tasks.push_back(task_id);

					auto new_task = make_shared<WaitTree>();
					new_task->code_id = code_id;
					new_task->task_id = task_id;
					auto parent_task = waittree_map[parent_id];
					new_task->parent = parent_task;
					waittree_map.insert({task_id, new_task});
					parent_task->add_child(new_task);
				}
				break;
			default:
				printf("re: received a unknown tag: %d\n", tag);
				break;
		}

		delete[](recv_buffer);

		while(free_workers > 0 && queued_tasks.size() > 0){
			int task_id = queued_tasks.front();
			queued_tasks.pop_front();
			
			shared_ptr<WaitTree> next_task = waittree_map[task_id];

			for(auto it = workers.begin(); it != workers.end(); it++){
				if(it->capacity > 0){
					it->capacity--;
					printf("re scheduling new task %d  with id %d on worker %d, remaining capacity = %d \n",
						 	next_task->code_id, task_id, it->node_id, it->capacity);
					run_task(task_id, next_task->code_id, it->node_id);
					break;
				}	
			}
			
			running_tasks.insert(task_id);
			free_workers--;
		}

		//check for termination:
		printf("re: running_tasks = %d, queued_tasks = %d, main_waiting = %d\n",
				running_tasks.size(), queued_tasks.size(), main_waiting);
		shared_ptr<WaitTree> root = waittree_map[root_id];
		if(root->not_finished_children == 0 && root->execution_finished){
			/*
		if(running_tasks.size() == 0
				&& queued_tasks.size() == 0
				&& main_waiting){*/
			printf("re: termination requirements fulfilled\n");
			for(worker_s w : workers){
				printf("re: sending quit to worker %d\n", w.node_id);
				quit(w.node_id);
			}
			printf("re: sending quit to main thread\n");
			quit(main_rank);
			quit_loop = true;
			printf("state of root: running_children: %d, not_finished_children: %d, total_children: %d, finished_execution: %d \n",
					root->running_children, root->not_finished_children, root->total_children, root->execution_finished);
		}
	}
	printf("re: event loop finished\n");
}
