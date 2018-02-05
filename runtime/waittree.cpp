#include <list>
#include <memory>
#include "waittree.h"

using namespace std;

void WaitTree::finished_execution(){
	printf("finished_execution called\n");
	execution_finished = true;
	if(not_finished_children == 0){
		if(parent != nullptr){
			parent->child_finished();
		} else {
			printf("finished_execution nullptr\n");
		}
	} else {
		printf("finished_execution else\n");
	}
}

void WaitTree::child_finished_execution(){
	printf("child_finished_execution called\n");
	printf("%d\n", running_children);
	running_children--;
	if((not_finished_children == 0 ) && (execution_finished == true)){
		if(parent != nullptr){
			parent->child_finished();
		} else {
			printf("child_finished_execution nullptr\n");
		}
	}
}

void WaitTree::child_finished(){
	printf("child_finished called\n");
	not_finished_children--;
	running_children--;
	if((not_finished_children == 0) && (execution_finished == true)){
		if(parent != nullptr){
			parent->child_finished();
		} else {
			printf("child finished nullptr\n");
		}
	}
}

void WaitTree::add_child(shared_ptr<WaitTree> child){
	children.push_back(weak_ptr<WaitTree>(child));
	running_children++;
	not_finished_children++;
	total_children++;
}
