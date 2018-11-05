#include <list>
#include <memory>
#include "RuntimeTask.h"
#include "mpi.h"

using namespace std;

void RuntimeTask::finished_execution(){
	//printf("finished_execution called\n");
	execution_finished = true;
	if(parent != nullptr){
		parent->child_finished_execution();
		if(not_finished_children == 0){
			parent->child_finished();
		} 
	} 
}

void RuntimeTask::child_finished_execution(){
	//printf("child_finished_execution called\n");
	running_children--;
	if(task_waiting && running_children == 0){
		handle_wakeup();
	}
	if((not_finished_children == 0 ) && (execution_finished == true)){
		if(parent != nullptr){
			parent->child_finished();
		} 
	}
}

void RuntimeTask::child_finished(){
	//printf("child_finished called\n");
	not_finished_children--;
	if(all_waiting && not_finished_children == 0){
		handle_wakeup();
	}
	if((not_finished_children == 0) && (execution_finished == true)){
		if(parent != nullptr){
			parent->child_finished();
		} 
	}
}

void RuntimeTask::add_child(shared_ptr<RuntimeTask> child){
	children.push_back(weak_ptr<RuntimeTask>(child));
	running_children++;
	not_finished_children++;
	total_children++;
}

void RuntimeTask::handle_wakeup(){
	MPI_Send(&task_id, 1, MPI_INT, waiting_node, TAG_WAKEUPTASK, MPI_COMM_WORLD);
	printf("re waking up task %d at node %d\n", task_id, waiting_node);
	waiting_node = -1;
	task_waiting = false;
	all_waiting = false;
}	
