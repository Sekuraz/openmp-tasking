#ifndef TDOMP_ROLES_H
#define TDOMP_ROLES_H
#include <atomic>
#include <vector>
#include <thread>
#include <list>
#include <memory>
#include <condition_variable>
#include <mutex>

class WorkerTask {
	public:
		WorkerTask();
		~WorkerTask();
		WorkerTask(const WorkerTask& other);
		int task_id;
		int code_id;
		int* task_descr;
		int task_descr_length;
		std::shared_ptr<std::atomic<bool>> finish_flag;
		std::shared_ptr<std::thread> task_thread;

		//need for taskwait/allwait
		std::shared_ptr<std::condition_variable> cv;
		std::shared_ptr<std::atomic<bool>> continue_flag;
		std::shared_ptr<std::mutex> m;
};

class Worker {
	public:
		std::list<WorkerTask> running_tasks;
		int worker_rank;
		int capacity;

		Worker();

		void task_wrapper_function(WorkerTask& task);
		void event_loop();
};

namespace re {
	void event_loop();

	struct worker_s {
		int capacity;
		int node_id;
	};
}

namespace main_thread {
	void event_loop();
}

#endif 
