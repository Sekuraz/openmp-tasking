#include "functions.h"
#include "roles.h"
#include "utils.h"

#include <cstdio>

using namespace std;
void main_thread::event_loop(){
	//printf("main thread event loop started\n");

	//create some tasks here
	//printf("main thread is creating a task\n");
	for(int i = 0; i < 12; i++){
		int task_descr = 1;
		create_task(&task_descr,1);
	}

	//printf("main thread waiting for all tasks to finish\n");	
	wait_all();
	//printf("main thread is waiting for finish signal\n");
	wait_quit();	
	//printf("main thread event loop finished\n");
}
