#ifndef DTOMP_WAITTREE_H
#define DTOMP_WAITTREE_H

#include <list>
#include <memory>
#include "utils.h"

class RuntimeTask{
	public:
		bool execution_finished = false;
		std::list<std::weak_ptr<RuntimeTask>> children;
		std::shared_ptr<RuntimeTask> parent;

		// Called by the corresponding task/section when it's execution is finished
		void finished_execution();
		// Called by a child, when it has finished it's execution
		void child_finished_execution();
		// Called by a child if all it's children and it's own execution is finished
		void child_finished();
		// Adds a child
		void add_child(std::shared_ptr<RuntimeTask> child);

		// handles the wakeup of a task
		void handle_wakeup();

		int running_children = 0;
		int not_finished_children = 0;
		int total_children = 0;

		node_id_t origin;
		code_id_t code_id;
		task_id_t task_id;

		// set to true if the task/section is currently in a wait state.
		// there can only be one type of waiting for a task at any given time
		// by only one node
		bool task_waiting = false;
		bool all_waiting = false;
		node_id_t waiting_node = -1;
};
#endif
