//
// Created by markus on 04.11.18.
//

#ifndef PROJECT_WORKERTASK_H
#define PROJECT_WORKERTASK_H

#include <condition_variable>
#include <mutex>
#include <atomic>
#include <thread>

#include "utils.h"

class WorkerNode;

void run_worker(int);

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
	void handle_create_from_main(code_id_t code_id, node_id_t node_id);

	void handle_wakeup(); //called to continue the task
	int claim_capacity(int desired);
	void return_capacity(int claimed);

	void run_task();

	//TODO variables missing
	// probably not?

	task_id_t task_id;
	code_id_t code_id;
	node_id_t origin;
	std::atomic<bool> finish_flag;
	std::thread task_thread;

	bool started = false;

	//needed for wakeup
	std::condition_variable cv;
	std::mutex m;
	std::atomic<bool> continueable;

	// needed to add things to the waiting_tasks map
	WorkerNode* worker;
};


#endif //PROJECT_WORKERTASK_H
