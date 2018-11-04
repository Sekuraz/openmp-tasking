#include <cstdio>
#include <chrono>
#include <memory>
#include "mpi.h"

#include <map>

#include "Worker.h"
#include "WorkerTask.h"

using namespace std;

void run_worker(int world_rank) {
	WorkerNode worker;
	worker.setup();
	worker.event_loop();
	printf("worker %d main finished\n", world_rank);
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
                case TAG_MEMORY_TRANSFER:
                    transfer_memory(recv_buffer);
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
	node_id_t origin = buffer.get()[2];

	tasks.insert({task_id, make_shared<WorkerTask>()});

	auto task = tasks[task_id];
	task->task_id = task_id;
	task->code_id = code_id;
	task->origin = origin;
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

void * WorkerNode::request_memory(node_id_t origin, task_id_t task_id) {
    array<int, 1> request {task_id};
    MPI_Send(&request, 1, MPI_INT, origin, TAG_MEMORY_TRANSFER, MPI_COMM_WORLD);

    return nullptr;
}

void WorkerNode::transfer_memory(std::shared_ptr<int> buffer) {

}
