#ifndef TDOMP_RUNTIME_H
#define TDOMP_RUNTIME_H

#include <map>
#include <memory>
#include <deque>
#include <mpi.h>

#include "utils.h"
#include "RuntimeTask.h"

class Worker{

	/*
	 * This class represents a worker as seen by the runtime Nodes.
	 */

	public:

	node_id_t node_id;
	int capacity;

	// sends the shutdown command to this worker.
	void shutdown();
};

class RuntimeNode {
	/* 
	 * This class represents a runtime node.
	 */ 

	std::map<node_id_t, std::shared_ptr<Worker>> workers; //contains all workers
	std::deque<std::shared_ptr<Worker>> free_workers; //contains workers that have free capacity
	std::map<task_id_t, std::shared_ptr<RuntimeTask>> waittree_map;
	std::map<task_id_t, std::shared_ptr<RuntimeTask>> running_tasks;
	std::deque<std::shared_ptr<RuntimeTask>> queued_tasks; //tasks that are ready to be executed

	public:

	node_id_t node_id;

	/*
	 * This takes care of setting up the environment.
	 * This means creating the implicit task.
	 */
	void setup();

	/*
	 * The event loop
	 */
	void event_loop();

	//handle functions. Called when the event occurs.
	//
	// these are called upon receiving a message
	void handle_create_task(std::shared_ptr<int> buffer, int length);
	void handle_finish_task(std::shared_ptr<int> buffer, int length);
	void handle_taskwait(std::shared_ptr<int> buffer, int length, node_id_t source);
	void handle_allwait(std::shared_ptr<int> buffer, int length, node_id_t source);
	void handle_free_capacity(std::shared_ptr<int> buffer, int length, node_id_t source);
	//this is called when something needs to be done
	void handle_run_task(); // finds the next task to be executed and a fitting
							// node to run it oo

	// receives a message and calls the corresponding handle
	void receive_message();

	// called when the programm needs to be terminated
	// also shuts down all workers belonging to the runtime node.
	void shutdown();

	void check_completion();

	private:
	bool quit_loop = false;

	int recv_pending = 0;
	bool recv_running = false;
	//int* recv_buffer;
	std::shared_ptr<int> recv_buffer;
	int recv_buffer_size;
	MPI_Status status;
	MPI_Request request;
	task_id_t id_counter = 0;
};


#endif
