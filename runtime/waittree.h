#ifndef DTOMP_WAITTREE_H
#define DTOMP_WAITTREE_H

#include <list>
#include <memory>

class WaitTree{
	public:
		bool execution_finished;
		std::list<std::weak_ptr<WaitTree>> children;
		std::shared_ptr<WaitTree> parent;

		// Called by the corresponding task/section when it's execution is finished
		void finished_execution();
		// Called by a child if all it's children and it's own execution is finished
		void child_finished();
		// Called by a child, when it has finished it's execution
		void child_finished_execution();
		// Adds a child
		void add_child(std::shared_ptr<WaitTree> child);

		int running_children = 0;
		int not_finished_children = 0;
		int total_children = 0;

		int code_id;
		int task_id;

		//TODO
		// expand start protocol with section ids
		// set to true if the task/section is currently in a wait state.
		bool waiting = false;
		// the node_id on which the task is currently running (suspended).
		int wait_node_id = -1;
};
#endif
