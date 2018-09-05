#ifndef TDOMP_FUNCTIONS_H
#define TDOMP_FUNCTIONS_H
#include "mpi.h"
void create_task(int* task_descr, int length);

void run_task(int task_id, int* task_descr, int length, int worker_id);

void finish_task(int task_id);

void quit(int target_rank);

void wait_quit();

void wait_all();
#endif
