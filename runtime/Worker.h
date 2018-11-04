#ifndef TDOMP_WORKER_H
#define TDOMP_WORKER_H

#include "utils.h"
#include "defaults.h"
#include "mpi.h"
#include <map>
#include <deque>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

class WorkerTask;

class WorkerNode {

	/*
	 * This represents a Worker.
	 */
	public:
	node_id_t node_id;
	std::map<task_id_t, std::shared_ptr<WorkerTask>> tasks;

	std::mutex lock;
	std::map<task_id_t, std::shared_ptr<WorkerTask>> waiting_tasks;
	std::deque<std::shared_ptr<WorkerTask>> runnable_tasks;

	std::atomic<int> capacity;
	// the capacity that was freed by suspending
	// This gets dumped into the regular capacity in the event loop
	std::atomic<int> waiting_capacity; 

	void setup();

	
	void handle_run_task(std::shared_ptr<int> buffer, int length);
	// tells the runtime node that now amount more capacity if free
	void handle_free_capacity(int amount = 1);
	// handles a shutdown message from the runtime
	// this means joining with all tasks and breaking the main loop
	void handle_shutdown();

	//called to move the suspended task to the runnable tasks
	void handle_wakeup(std::shared_ptr<int> buffer, int length);

	void event_loop();

	// called to receive a message
	// This is called frequently by the event loop.
	// This should not be blocking
	// This might not do something on every call, depending on if a message has
	// been received.
	void receive_message();

	//adds task id to waiting_tasks.
	//this is needed to continue the task later.
	void add_to_waiting(task_id_t task_id);

	void *request_memory(node_id_t i, task_id_t i1);

private:
	bool quit_loop = false;

	int recv_pending = 0;
	bool recv_running = false;
	//int* recv_buffer;
	std::shared_ptr<int> recv_buffer;
	int recv_buffer_size;
	MPI_Status status;
	MPI_Request request;

	void transfer_memory(std::shared_ptr<int> shared_ptr);
};

#endif
