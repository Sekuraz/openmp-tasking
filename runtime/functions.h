#ifndef TDOMP_FUNCTIONS_H
#define TDOMP_FUNCTIONS_H
#include "mpi.h"
void create_task(int parent_id, int code_id);

void run_task(int task_id, int code_id, int worker_id);

void finish_task(int task_id);

void quit(int target_rank);

void wait_quit();

void wait_all();

//TODO new functions for taskwait, sections and sectionwait and a function (or multiple) to wake up the threads again.
#endif
