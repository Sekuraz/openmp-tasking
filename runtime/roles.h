#ifndef TDOMP_ROLES_H
#define TDOMP_ROLES_H
#include <atomic>
#include <vector>
#include <thread>
#include <list>
#include <memory>

class WorkerTask {
	public:
		WorkerTask();
		~WorkerTask();
		WorkerTask(const WorkerTask& other);
		int task_id;
		int* task_descr;
		int task_descr_length;
		std::shared_ptr<std::atomic<bool>> finish_flag;
		std::shared_ptr<std::thread> task_thread;
};

class Worker {
	public:
		std::vector<std::atomic<bool>> task_flags;
		std::vector<std::thread> task_threads;
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
