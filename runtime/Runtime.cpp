#include "utils.h"
#include "mpi.h"
#include "defaults.h"
#include "Runtime.h"
#include <thread>
#include <chrono>

using namespace std;

void Worker::shutdown(){
	printf("Runtime is sending shutdown to %d\n", node_id);
	MPI_Send(nullptr, 0, MPI_INT, node_id, TAG_SHUTDOWN, MPI_COMM_WORLD);
};

void RuntimeNode::setup(){
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	node_id = world_rank;

	int world_size = 0;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	for( int i = 1; i < world_size; i++){
		auto w = make_shared<Worker>();
		w->node_id = i;
		w->capacity = default_capacity;
		workers.insert({i, w});
		free_workers.push_back(w);
	}

	// initialer Task bauen
	task_id_t root_id = id_counter++;
	waittree_map.insert({root_id, make_shared<RuntimeTask>()});
	
	auto root_task = waittree_map[root_id];
	root_task->code_id = 0;
	root_task->task_id = root_id;

	queued_tasks.push_back(root_task);
}

/*
 * The event loop
 */
void RuntimeNode::event_loop(){
	printf("Runtime in event loop\n");
	while(!quit_loop){
		//printf("Runtime event loop\n");
		receive_message();
		while(free_workers.size() != 0 && queued_tasks.size() != 0){
			handle_run_task();
		}
		check_completion();

		//sleep a bit so we are not spamming the logs to much
		this_thread::sleep_for(chrono::milliseconds(10));
	}
	shutdown();
}

//handle functions. Called when the event occurs.
//
// these are called upon receiving a message
void RuntimeNode::handle_create_task(std::shared_ptr<int> buffer, int length){
	int* data = buffer.get();
	task_id_t parent_id = data[0];
	code_id_t code_id = data[1];
	task_id_t new_task_id = id_counter++;
	waittree_map.insert({new_task_id, make_shared<RuntimeTask>()});

	auto new_task = waittree_map[new_task_id];
	auto parent_task = waittree_map[parent_id];

	new_task->code_id = code_id;
	new_task->task_id = new_task_id;
	new_task->parent = parent_task;

	parent_task->add_child(new_task);

	queued_tasks.push_back(new_task);
}

void RuntimeNode::handle_finish_task(std::shared_ptr<int> buffer, int length){
	task_id_t task_id = buffer.get()[0];
	waittree_map[task_id]->finished_execution();
}

void RuntimeNode::handle_taskwait(std::shared_ptr<int> buffer, int length, node_id_t source){
	task_id_t task_id = buffer.get()[0];
	waittree_map[task_id]->task_waiting = true;
	waittree_map[task_id]->waiting_node = source;
	printf("runtime %d task %d is waiting now\n", node_id, task_id);
}

void RuntimeNode::handle_allwait(std::shared_ptr<int> buffer, int length, node_id_t source){
	task_id_t task_id = buffer.get()[0];
	waittree_map[task_id]->all_waiting = true;
	waittree_map[task_id]->waiting_node = source;
	printf("runtime %d task %d is all waiting now\n", node_id, task_id);
}

void RuntimeNode::handle_free_capacity(std::shared_ptr<int> buffer, int length, node_id_t source){
	int amount = buffer.get()[0];
	auto worker = workers[source];
	if(worker->capacity == 0){
		free_workers.push_back(worker);
	}
	worker->capacity += amount;
}

// this is called when something needs to be done
void RuntimeNode::handle_run_task(){
	// finds the next task to be executed and a fitting node to run it on

	// check if there are tasks and workers available:
	if(queued_tasks.size() == 0 || free_workers.size() == 0){
		printf("handle_run_task got called but no tasks or free workers were available\n");
		return;
	}

	auto task = queued_tasks.front();
	queued_tasks.pop_front();
	auto worker = free_workers.front();
	worker->capacity -= 1;
	if (worker->capacity == 0){
		free_workers.pop_front();
	}
	
	int buffer[2];
	buffer[0] = task->task_id;
	buffer[1] = task->code_id;

	MPI_Send(&buffer, 2, MPI_INT, worker->node_id, TAG_COMMAND, MPI_COMM_WORLD);
}

// receives a message and calls the corresponding handle
void RuntimeNode::receive_message(){
	int tag;
	int source;

	if(! recv_running){
		//printf("runtime %d recv is not running. probing...\n", node_id);
		MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
				&recv_pending, &status);
	} 

	if(recv_pending != 0){
		//printf("runtime %d started receiving\n", node_id);
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
				case TAG_CREATE:
					handle_create_task(recv_buffer, recv_buffer_size);
					break;
				case TAG_COMMAND:
					handle_finish_task(recv_buffer, recv_buffer_size);
					break;
				case TAG_WAITTASK:
					handle_taskwait(recv_buffer, recv_buffer_size, source);
					break;
				case TAG_WAITALL:
					handle_allwait(recv_buffer, recv_buffer_size, source);
					break;
				case TAG_FREECAPACITY:
					handle_free_capacity(recv_buffer, recv_buffer_size, source);
					break;
				default:
					printf("runtime %d received a message with the unknown tag: %d.\n",
							node_id, tag);
					break;
			}
			recv_buffer.reset();
		}
	}
}

// called when the programm needs to be terminated
// also shuts down all workers belonging to the runtime node.
void RuntimeNode::shutdown(){
	for(auto it = workers.begin(); it != workers.end(); it++){
		it->second->shutdown();
	}
}

void RuntimeNode::check_completion(){
	//checks if the initial task and all it's children has finished.
	auto initial_task = waittree_map[0];
	quit_loop = (initial_task->execution_finished 
			&& initial_task->not_finished_children == 0);
}
