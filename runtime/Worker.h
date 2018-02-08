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

class WorkerNode;

class WorkerTask {
	/*
	 * This represents a Task that is running on a WorkerNode
	 */

	public:

	void handle_taskwait(); //wrapper around handle_wait
	void handle_allwait(); //wrapper around handle_wait
	void handle_wait(int tag); //can handle waiting but with different tags
	void handle_finish();
	void handle_create(code_id_t code_id);
	void handle_wakeup(); //called to continue the task

	void run_task();
	
	//TODO variables missing
	// probably not?
	
	task_id_t task_id;
	code_id_t code_id;
	std::atomic<bool> finish_flag; 
	std::thread task_thread;

	//needed for wakeup
	std::condition_variable cv;
	std::mutex m;
	std::atomic<bool> continueable;

	// needed to add things to the waiting_tasks map
	WorkerNode* worker;
};

class WorkerNode {

	/*
	 * This represents a Worker.
	 */
	public:
	node_id_t node_id;
	std::map<task_id_t, std::shared_ptr<WorkerTask>> tasks;

	std::mutex lock;
	std::map<task_id_t, std::shared_ptr<WorkerTask>> waiting_tasks;
	std::deque<std::shared_ptr<WorkerTask>> continueable_tasks;

	int capacity = default_capacity;

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

	private:
	bool quit_loop = false;

	int recv_pending = 0;
	bool recv_running = false;
	//int* recv_buffer;
	std::shared_ptr<int> recv_buffer;
	int recv_buffer_size;
	MPI_Status status;
	MPI_Request request;
	
	
};

#endif
