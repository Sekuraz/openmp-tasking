#include "functions.h"
#include "roles.h"
#include "utils.h"

#include <cstdio>

using namespace std;
void main_thread::event_loop(){
	//printf("main thread event loop started\n");
	//
	int parent_id = 0;
	int code_id = 0;

	//create some tasks here
	//printf("main thread is creating a task\n");
	for(int i = 0; i < 4; i++){
		create_task(parent_id, code_id);
	}
	code_id = 1;
	for(int i = 0; i < 4; i++){
		create_task(parent_id, code_id);
	}

	finish_task(parent_id);

	//printf("main thread waiting for all tasks to finish\n");	
	wait_all();
	//printf("main thread is waiting for finish signal\n");
	wait_quit();	
	//printf("main thread event loop finished\n");
}
