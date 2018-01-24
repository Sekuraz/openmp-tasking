#include "functions.h"
#include "roles.h"
#include "utils.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "mpi.h"
#include <thread>
#include <chrono>
#include <functional>

#include <deque>
#include <set>
#include <vector>
#include <list>

using namespace std;
WorkerTask::WorkerTask(){
}

WorkerTask::~WorkerTask(){
	//free(task_descr);
}

WorkerTask::WorkerTask(const WorkerTask& other){
	task_descr = other.task_descr;
	task_descr_length = other.task_descr_length;
	task_id = other.task_id;
	finish_flag = other.finish_flag;
	task_thread = other.task_thread;
}


worker::worker(){
	MPI_Comm_rank(MPI_COMM_WORLD, &worker_rank);
};

void worker::task_wrapper_function(atomic<bool> &finish_flag, int* task_descr, int task_descr_length, int task_id){
	printf("worker %d running task %d\n", worker_rank, task_id);
	//sleep some time to do some "work"
	//int sleep_time = (rand() / (float) RAND_MAX) * 1000;
	int sleep_time = 1000;
	this_thread::sleep_for(chrono::milliseconds(sleep_time));
	/*vector<double> a;

	for(int i = 0; i < 1024*1024; i++){
		a.push_back(i);
	}

	for(int rounds = 0; rounds < 512; rounds++){
		for(int i = 0; i < 1024*1024; i++){
			a[i] *=2;
		}
	}
	*/

	printf("worker %d finished task %d\n", worker_rank, task_id);

	finish_flag = true;
}

void worker::event_loop(){

	srand(time(nullptr));

	// Integer Flags for IRecv and IProbe
	int recv_pending = 0; // is there something to receive (used as flag for MPI_Iprobe)
  bool recv_running = false; // is there a Irecv currently running 

	bool quit_loop = false;
	capacity = 4;

	MPI_Comm_rank(MPI_COMM_WORLD, &(this->worker_rank));
	MPI_Status status;
	MPI_Request request;

	int* recv_buffer;
	int recv_buffer_size;

	int tag;
	int source;

	while(!quit_loop){

		if(! recv_running){
			//printf("worker %d recv is not running!\n", worker_rank);
			//printf("worker %d started probing\n", worker_rank);
			MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_pending, &status);
		} 

		if(recv_pending != 0){
			//printf("worker %d started receiving\n", worker_rank);
			// start recv
			MPI_Get_count(&status, MPI_INT, &recv_buffer_size);
			tag = status.MPI_TAG;
			source = status.MPI_SOURCE;
			recv_buffer = (int*) malloc(recv_buffer_size*sizeof(int));
			MPI_Irecv(recv_buffer, recv_buffer_size, MPI_INT, source, tag, MPI_COMM_WORLD, &request);
			recv_pending = 0;
			recv_running = true;
		} 
		if (recv_running){
			//test if recv has finished
			int recv_finish_flag = 0;
			MPI_Test(&request, &recv_finish_flag, MPI_STATUS_IGNORE);
			if (recv_finish_flag != 0){
				recv_running = false;

				tag = status.MPI_TAG;
				source = status.MPI_SOURCE;
				switch(tag){
					case TAG_COMMAND:
						{
							printf("worker %d received COMMAND signal\n", worker_rank);
							WorkerTask task;
							task.task_id = recv_buffer[0];
							task.task_descr = recv_buffer+1;
							task.task_descr_length = recv_buffer_size -1;
							task.finish_flag = new atomic<bool>{false};
							task.task_thread = new thread(&worker::task_wrapper_function, this, std::ref(*(task.finish_flag)), task.task_descr, task.task_descr_length, task.task_id);
							printf("worker started task %d\n", task.task_id);
							running_tasks.push_back(task);
						}
						break;
					case TAG_QUIT:
						printf("worker %d received QUIT signal\n", worker_rank);
						quit_loop = true;
						break;
					default:
						printf("worker %d received a message with the unknown tag: %d.\n", worker_rank, tag);
						break;
				}
				free(recv_buffer);
			}
		}

		//Check if other tasks have finished
	
		auto it = running_tasks.begin();
		while(it != running_tasks.end()){
			if(*(it->finish_flag) == true){
				//task has finished
				printf("worker %d is joining with task %d\n", worker_rank, it->task_id);
				it->task_thread->join();
				finish_task(it->task_id);
				printf("worker %d is joined with task %d\n", worker_rank, it->task_id);
				it = running_tasks.erase(it);
			} else {
			 	it++;
			}
		}

		//sleep some time
		this_thread::sleep_for(chrono::milliseconds(10));
	}
	printf("worker %d event loop finished\n", worker_rank);
}


void main_thread::event_loop(){
	//printf("main thread event loop started\n");

	//create some tasks here
	//printf("main thread is creating a task\n");
	for(int i = 0; i < 8; i++){
		int task_descr = 1;
		create_task(&task_descr,1);
	}

	//printf("main thread waiting for all tasks to finish\n");	
	wait_all();
	//printf("main thread is waiting for finish signal\n");
	wait_quit();	
	//printf("main thread event loop finished\n");
}

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
				printf("re received a finished_task from worker %d\n", source);
				free_workers++;
				for(int i = 0; i < workers.size(); i++){
					if(workers[i].node_id == source){
						workers[i].capacity+=1;
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

		free(recv_buffer);

		while(free_workers > 0 && queued_tasks.size() > 0){
			int task_id = task_id_counter;
			task_id_counter++;

			int task_descr = queued_tasks.front();
			queued_tasks.pop_front();
			
			for(int i = 0; i < workers.size(); i++){
				if(workers[i].capacity > 0){
					workers[i].capacity-=1;
					printf("re scheduling new task with id %d on worker %d, remaining capacity = %d \n", task_id, workers[i].node_id, workers[i].capacity);
					run_task(task_id, &task_descr, 1, workers[i].node_id);
					break;
				} 
			}
			running_tasks.insert(task_id);
			free_workers--;
		}

		//check for termination:
		if(running_tasks.size() == 0
				& queued_tasks.size() == 0
				& main_waiting){
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
