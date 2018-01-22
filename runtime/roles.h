#ifndef TDOMP_ROLES_H
#define TDOMP_ROLES_H
#include <atomic>
#include <vector>
#include <thread>
class worker {
	public:
		std::vector<std::atomic<bool>> task_flags;
		std::vector<std::thread> task_threads;
		int worker_rank;

		worker();

		void task_wrapper_function(std::atomic<bool>& finish_flag, int* task_descr, int task_descr_length);
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
