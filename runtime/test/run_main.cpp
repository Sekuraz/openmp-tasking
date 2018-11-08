//
// Created by markus on 05.11.18.
//

#include <mpi.h>
#include <thread>
#include <chrono>
#include <iomanip>


#include "../src/globals.h"
#include "../src/Runtime.h"
#include "../src/Worker.h"
#include "../src/Task.h"

using namespace std;

void __main__(int argc, char ** argv) {
    cout << "MAIN IS RUNNING" << endl;
    current_task->worker->handle_finish_task(current_task);
}


int main(int argc, char ** argv) {
    MPI_Init(nullptr, nullptr);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_size < 2) {
        cout << "This code MUST be run by mpirun!" << endl;
        exit(EXIT_FAILURE);
    }

    if (world_rank == 0) {
        auto r = make_shared<Runtime>(world_rank, world_size);
        r->setup();
        auto node_and_task = r->scheduler.get_next_node_and_task();
        r->run_task_on_node(node_and_task.second, node_and_task.first);

        while (true) {
            auto m = r->receive_message();

            if (m.tag == TAG::FINISH_TASK) {
                r->shutdown();
                break;
            }
        }
    }
    else {
        auto w = make_shared<Worker>(world_rank);
        w->setup();

        while (true) {
            auto m = w->receive_message();

            if (m.tag == TAG::RUN_TASK) {
                auto task = Task::deserialize(&m.data[0]);
                w->handle_run_task(task);
            }
            if (m.tag == TAG::SHUTDOWN) {
                break;
            }
        }
    }

    MPI_Finalize();
}