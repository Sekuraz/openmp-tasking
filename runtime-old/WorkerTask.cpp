//
// Created by markus on 04.11.18.
//

#include "WorkerTask.h"
#include "Worker.h"

using namespace std;

extern thread_local WorkerTask* current_task;
extern std::map<int, void (*)(void **)> tasking_function_map;

extern int argc;
extern char** argv;
void __main_0(int, char**);


void WorkerTask::handle_taskwait(){
	handle_wait(TAG_WAITTASK);
}

void WorkerTask::handle_allwait(){
	handle_wait(TAG_WAITALL);
}

void WorkerTask::handle_wait(int tag){
	worker->add_to_waiting(task_id);
	MPI_Send(&task_id, 1, MPI_INT, re_rank, tag, MPI_COMM_WORLD);
	{
		unique_lock<mutex> lk(m);
		while(!continueable.load()){
			cv.wait(lk);
		}
		continueable = false;
	}
	printf("task %d continues\n", task_id);
}

void WorkerTask::handle_finish(){
	MPI_Send(&task_id, 1, MPI_INT, re_rank, TAG_COMMAND, MPI_COMM_WORLD);
	finish_flag = true;
}

void WorkerTask::handle_create(code_id_t code_id){
	int buffer[3];
	buffer[0] = task_id;
	buffer[1] = code_id;
	buffer[2] = worker->node_id;
	MPI_Send(&buffer, 3, MPI_INT, re_rank, TAG_CREATE, MPI_COMM_WORLD);
}

void WorkerTask::handle_create_from_main(code_id_t code_id, node_id_t node_id){
	int buffer[3];
	buffer[0] = task_id;
	buffer[1] = code_id;
	buffer[2] = node_id;
	MPI_Send(&buffer, 3, MPI_INT, re_rank, TAG_CREATE, MPI_COMM_WORLD);
}

void WorkerTask::handle_wakeup(){
	{
		lock_guard<mutex> lk(m);
		continueable = true;
	}
	cv.notify_one();
	printf("notified task %d to wake up\n", task_id);
}

int WorkerTask::claim_capacity(int desired){

	if(worker->capacity <= 0){
		return 0;
	} else {
		int result = worker->capacity -= desired;
		if(result >= 0){ //got the desired capacity
			return desired;
		} else { //claimed "to much"
			//the result contains how much has been taken to much (as negative number)
			//now add it back up
			worker->capacity -= result;
		}
		// + here because result is allways negative at this point
		return desired + result;
	}
}

void WorkerTask::return_capacity(int claimed){
	worker->capacity += claimed;
}

void WorkerTask::run_task(){
	started = true;
	printf("task %d with code_id %d is running!\n", task_id, code_id);

	current_task = this;

	if (code_id == 0) {
		__main_0(argc, argv);
	} else if (tasking_function_map.count(code_id)) {
	    void * arguments = worker->request_memory(origin, task_id);
    } else {
        printf("task %d unknown code id %d\n", task_id, code_id);
    }

    this_thread::sleep_for(chrono::milliseconds(1000));

    printf("task %d with code_id %d has finished!\n", task_id, code_id);

	handle_finish();
}