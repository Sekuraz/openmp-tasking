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
	if(worker == nullptr){
		printf("error weak_ptr returned nullptr\n");
	}
	worker->add_to_waiting(task_id);
	MPI_Send(&task_id, 1, MPI_INT, re_rank, TAG_WAITTASK, MPI_COMM_WORLD);
	{
		unique_lock<mutex> lk(m);
		while(!continueable.load()){
			cv.wait(lk);
		}
		continueable = false;
	}
	printf("task %d continues\n", task_id);
}

void WorkerTask::handle_allwait(){
	MPI_Send(&task_id, 1, MPI_INT, re_rank, TAG_WAITALL, MPI_COMM_WORLD);
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
	printf("trying to get the lock\n");
	printf("continueable is %d\n", continueable.load());
	continueable = true;
	{
		//lock_guard<mutex> lk(m);
		printf("got the lock\n");
		continueable.store(true);
	}	
	printf("set the variable\n");
	cv.notify_one();
	printf("notified task %d to wake up\n", task_id);
}

void WorkerTask::run_task(){
	//TODO
	//This starts the task
	printf("task %d with code_id %d is running!\n", task_id, code_id);

	switch(code_id){
		case (0):
			for(int i = 0; i < 4; i++){
				handle_create(1);
			}
			break;
		case (1):
			for(int i = 0; i < 4; i++){
				handle_create(2);
			}
			handle_taskwait();
			this_thread::sleep_for(chrono::seconds(1));
			break;
		case (2):
			this_thread::sleep_for(chrono::seconds(1));
			break;
		default:
			printf("task %d unknown code id %d\n", task_id, code_id);
	}

//	finish_flag = true;
	handle_finish();
}



void WorkerNode::setup(){

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	node_id = world_rank;
	printf("Worker %d has finished setup\n", node_id);
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
	
	//check for completion of tasks
	int free_capacity = 0;
	auto it = tasks.begin();
	while(it != tasks.end()){
		//peerhaps can be optimized
		if(it->second->finish_flag){
			printf("worker %d joining with task %d \n", node_id, it->first);
			it->second->task_thread.join();
			free_capacity++;
			printf("worker %d joined with task %d \n", node_id, it->first);
			it = tasks.erase(it);
		} else {
			it++;
		}
	}

	//TODO start suspended tasks
	while(free_capacity > 0 && continueable_tasks.size() > 0){
		auto task = continueable_tasks.front();
		continueable_tasks.pop_front();
		task->handle_wakeup();
		free_capacity--;
	}

	if(!quit_loop && free_capacity != 0){
		handle_free_capacity(free_capacity);
	}
}

void WorkerNode::handle_shutdown(){
	//TODO wegmachen?
	printf("worker %d received shutdown\n", node_id);
	quit_loop = true;
};

void WorkerNode::event_loop(){
	while(!quit_loop){
		//printf("worker %d event loop\n", node_id);
		receive_message();
		//printf("Worker %d is sleeping\n", node_id);
		this_thread::sleep_for(chrono::milliseconds(10));
	}
	printf("WorkerNode %d leaving event loop\n", node_id);
}

void WorkerNode::handle_run_task(std::shared_ptr<int> buffer, int length){
	capacity--;

	task_id_t task_id = buffer.get()[0];
	code_id_t code_id = buffer.get()[1];

	tasks.insert({task_id, make_shared<WorkerTask>()});

	auto task = tasks[task_id];
	task->task_id = task_id;
	task->code_id = code_id;
	task->worker = this;

	task->task_thread = thread(&WorkerTask::run_task, task);
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
	continueable_tasks.push_back(task);
	printf("worker %d received wakeup for task %d\n", node_id, task_id);
	if (task == nullptr){
		printf("nullptr\n");
	} else {
		printf("not nullptr\n");
	}
	task->continueable = true;
	task->handle_wakeup();
}

void WorkerNode::add_to_waiting(task_id_t task_id){
	lock_guard<mutex> lk(lock);
	auto task = tasks[task_id];
	waiting_tasks[task_id] = task;
}
