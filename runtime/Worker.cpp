#include <cstdio>
#include <chrono>
#include <thread>
#include <memory>
#include "mpi.h"

#include <map>

#include "utils.h"
#include "Worker.h"

using namespace std;

void WorkerTask::handle_taskwait(){
	handle_wait(TAG_WAITTASK);
}

void WorkerTask::handle_allwait(){
	handle_wait(TAG_WAITALL);
}

void WorkerTask::handle_wait(int tag){
	worker->add_to_waiting(task_id);
	MPI_Send(&task_id, 1, MPI_INT, re_rank, tag, MPI_COMM_WORLD);
	{
		unique_lock<mutex> lk(m);
		while(!continueable.load()){
			cv.wait(lk);
		}
		continueable = false;
	}
	printf("task %d continues\n", task_id);
}

void WorkerTask::handle_finish(){
	MPI_Send(&task_id, 1, MPI_INT, re_rank, TAG_COMMAND, MPI_COMM_WORLD);
	finish_flag = true;
}

void WorkerTask::handle_create(code_id_t code_id){
	int buffer[2];
	buffer[0] = task_id;
	buffer[1] = code_id;
	MPI_Send(&buffer, 2, MPI_INT, re_rank, TAG_CREATE, MPI_COMM_WORLD);
}

void WorkerTask::handle_wakeup(){
	{
		lock_guard<mutex> lk(m);
		continueable = true;
	}	
	cv.notify_one();
	printf("notified task %d to wake up\n", task_id);
}

int WorkerTask::claim_capacity(int desired){

	if(worker->capacity <= 0){
		return 0;
	} else {
		int result = worker->capacity -= desired;
		if(result >= 0){ //got the desired capacity
			return desired;
		} else { //claimed "to much"
			//the result contains how much has been taken to much (as negative number)
			//now add it back up
			worker->capacity -= result;
		}
		// + here because result is allways negative at this point
		return desired + result;
	}
}

void WorkerTask::return_capacity(int claimed){
	worker->capacity += claimed;
}

void WorkerTask::run_task(){
	started = true;
	printf("task %d with code_id %d is running!\n", task_id, code_id);

	switch(code_id){
		case (0):
			for(int i = 0; i < 4; i++){
				handle_create(1);
			}
			handle_allwait();
			break;
		case (1):
			for(int i = 0; i < 4; i++){
				handle_create(2);
			}
			handle_taskwait();
			printf("taskwait wake up\n");
			this_thread::sleep_for(chrono::seconds(1));
			break;
		case (2):
			{
				int claimed = claim_capacity(2);
				printf("claimed %d, remaining capacity is %d\n", claimed, worker->capacity.load());
				#pragma omp parallel num_threads(claimed+1)
				printf("parallel print\n");

				return_capacity(claimed);
			}
			this_thread::sleep_for(chrono::seconds(1));
			break;
		default:
			printf("task %d unknown code id %d\n", task_id, code_id);
	}

	handle_finish();
}

void WorkerNode::setup(){

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	node_id = world_rank;
	printf("Worker %d has finished setup\n", node_id);
	capacity = default_capacity;
}

void WorkerNode::receive_message(){
	int tag;
	int source;

	if(! recv_running){
		//printf("worker %d recv is not running. probing...\n", node_id);
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
				&recv_pending, &status);
	} 

	if(recv_pending != 0){
		printf("worker %d started receiving\n", node_id);
		MPI_Get_count(&status, MPI_INT, &recv_buffer_size);
		tag = status.MPI_TAG;
		source = status.MPI_SOURCE;
		recv_buffer = shared_ptr<int>(new int[recv_buffer_size], default_delete<int[]>());
		MPI_Irecv(recv_buffer.get(), recv_buffer_size, MPI_INT, source, tag, MPI_COMM_WORLD, &request);
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
					handle_run_task(recv_buffer, recv_buffer_size);
					break;
				case TAG_WAKEUPTASK:
					handle_wakeup(recv_buffer, recv_buffer_size);
					break;
				case TAG_SHUTDOWN:
					handle_shutdown();
					break;
				default:
					printf("worker %d received a message with the unknown tag: %d.\n",
							node_id, tag);
					break;
			}
			recv_buffer.reset();
		}
	}
	

}

void WorkerNode::handle_shutdown(){
	printf("worker %d received shutdown\n", node_id);
	quit_loop = true;
};

void WorkerNode::event_loop(){
	while(!quit_loop){
		//printf("worker %d event loop\n", node_id);
		receive_message();

		int start_capacity = capacity;

		//Handle the capacity that was freed by suspending tasks
		int capacity_dump = 0;
		capacity_dump = waiting_capacity.exchange(capacity_dump);
		capacity += capacity_dump;

		// TODO put this code in a function
		//check for completion of tasks
		auto it = tasks.begin();
		while(it != tasks.end()){
			if(it->second->finish_flag){
				it->second->task_thread.join();
				capacity++;
				it = tasks.erase(it);
			} else {
				it++;
			}
		}

		while(runnable_tasks.size() > 0){
			//Take the next task and execute it
			int remaining_capacity = --capacity;

			if(remaining_capacity < 0){ //We have taken more capacity than available
				capacity++; //return the capacity
				break;
			}

			auto task = runnable_tasks.front();
			runnable_tasks.pop_front();

			if(task->started == true){
				//Task was waiting and needs to be continued
				task->handle_wakeup();
			} else {
				//Task is new and needs to be started
				task->started = true;
				task->task_thread = thread(&WorkerTask::run_task, task);
			}
		}

		int free_capacity = capacity - start_capacity;

		// finaly free up the capacity
		if(!quit_loop && free_capacity != 0){
			handle_free_capacity(free_capacity);
		}
		this_thread::sleep_for(chrono::milliseconds(100));
	}
	printf("WorkerNode %d leaving event loop\n", node_id);
}

void WorkerNode::handle_run_task(std::shared_ptr<int> buffer, int length){
	task_id_t task_id = buffer.get()[0];
	code_id_t code_id = buffer.get()[1];

	tasks.insert({task_id, make_shared<WorkerTask>()});

	auto task = tasks[task_id];
	task->task_id = task_id;
	task->code_id = code_id;
	task->worker = this;

	runnable_tasks.push_back(task);
}

void WorkerNode::handle_free_capacity(int amount){
	printf("freeing capacity %d\n", amount);
	MPI_Send(&amount, 1, MPI_INT, re_rank, TAG_FREECAPACITY, MPI_COMM_WORLD);
}

void WorkerNode::handle_wakeup(std::shared_ptr<int> buffer, int length){
	lock_guard<mutex> lk(lock);
	printf("size of waiting tasks %d\n", waiting_tasks.size());
	task_id_t task_id = buffer.get()[0];
	auto task = waiting_tasks[task_id];
	waiting_tasks.erase(task_id);
	runnable_tasks.push_back(task);
}

void WorkerNode::add_to_waiting(task_id_t task_id){
	lock_guard<mutex> lk(lock);
	auto task = tasks[task_id];
	waiting_tasks[task_id] = task;
	waiting_capacity++;
}
