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

#include <set>
#include <vector>
#include <deque>

using namespace std;
WorkerTask::WorkerTask(){
}

WorkerTask::~WorkerTask(){
	delete[](task_descr);
}

WorkerTask::WorkerTask(const WorkerTask& other){
	task_descr = new int[other.task_descr_length]; 
	copy(other.task_descr, other.task_descr + other.task_descr_length, task_descr);
	task_descr_length = other.task_descr_length;
	task_id = other.task_id;

	//flat copies are intended here
	finish_flag = other.finish_flag;
	task_thread = other.task_thread;
	cv = other.cv;
	m = other.m;
	continue_flag = other.continue_flag;
}


Worker::Worker(){
	MPI_Comm_rank(MPI_COMM_WORLD, &worker_rank);
};

void Worker::task_wrapper_function(WorkerTask& task){
	printf("worker %d running task %d with code_id %d\n", worker_rank, task.task_id, task.code_id);
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
	/*if(task.code_id == 1){
	 *
	 * finish_task(parent_id);
		create_task(task.task_id,0);
	}*/

	printf("worker %d finished task %d\n", worker_rank, task.task_id);

	*(task.finish_flag) = true;
}

void Worker::event_loop(){

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
			recv_buffer = new int[recv_buffer_size];
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
							//printf("worker %d received COMMAND signal\n", worker_rank);
							running_tasks.emplace_back();
							WorkerTask& task = running_tasks.back();
							task.task_id = recv_buffer[0];
							task.code_id = recv_buffer[1];
							task.finish_flag.reset(new atomic<bool>{false});
							task.task_thread.reset(new thread(
										&Worker::task_wrapper_function,
									 	this, std::ref(task)));
							printf("worker %d started task %d\n",
									worker_rank, task.task_id);
						}
						break;
					case TAG_QUIT:
						printf("worker %d received QUIT signal\n", worker_rank);
						quit_loop = true;
						break;
					default:
						printf("worker %d received a message with the unknown tag: %d.\n",
								worker_rank, tag);
						break;
				}
				delete[](recv_buffer);
			}
		}

		//Check if other tasks have finished
		auto it = running_tasks.begin();
		while(it != running_tasks.end()){
			if(*(it->finish_flag) == true){
				//task has finished
				//printf("worker %d is joining with task %d\n", worker_rank, it->task_id);
				it->task_thread->join();
				finish_task(it->task_id);
				//printf("worker %d is joined with task %d\n", worker_rank, it->task_id);
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
