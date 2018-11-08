
#include <mpi.h>
#include <iostream>

#define AS 100

using namespace std;

#include <memory>

#include "tasking.h"

void x_1494453934 (void* arguments[]) {
    void * a_pointer_1 = arguments[0];
    void * a_pointer_0 = &(a_pointer_1);
    int * a = *((int **) a_pointer_0);
    void * i_pointer_0 = arguments[1];
    int i = *((int*) i_pointer_0);
    void * p_pointer_1 = arguments[2];
    void * p_pointer_0 = &(p_pointer_1);
    int * p = *((int **) p_pointer_0);

    {
            a[i] = i + *p;
    }
    current_task->worker->handle_finish_task(current_task);
}

int setup_1494453934() {
    tasking_function_map[1494453934] = &x_1494453934;
    return 1;
}

int tasking_setup_1494453934 = setup_1494453934();

void test(int a[AS], int* p) {
    for(int i = 0; i < AS; i++) {
        auto t_1494453934 = std::make_shared<Task>(1494453934);
        t_1494453934->if_clause = (i == 3);
        t_1494453934->final = (i == 5);
        t_1494453934->untied = true;
        t_1494453934->mergeable = true;
        {
            Var a_var = {"a", &(*a), at_firstprivate, 40000, 0};
            t_1494453934->vars.emplace_back(a_var);
        }
        {
            Var i_var = {"i", &(i), at_firstprivate, 4, 1};
            t_1494453934->vars.emplace_back(i_var);
        }
        {
            Var p_var = {"p", &(*p), at_firstprivate, 0, 0};
            t_1494453934->vars.emplace_back(p_var);
        }
        t_1494453934->in.emplace_back("a");

        current_task->worker->handle_create_task(t_1494453934);
    }
}


int * __array__; // in order to test the values after completion
int cc;

void __main__(int argc, char *argv[]) {

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (world_size != 2) {
        cout << "This code MUST be run by mpirun with 2 processes (no memory transfer)" << endl;
        exit(EXIT_FAILURE);
    }

    {
        cout << "hello from main" << endl;

        __array__ = new int[AS];
        cc = 2;
        int* p = &cc;

        test(__array__, p);

        cout << "good bye from main" << endl;

        return;
    }
}
int main(int argc, char** argv) {
    do_tasking(argc, argv);

    auto e = EXIT_SUCCESS;
    for (int i = 0; i < AS; i++) {
        if (__array__[i] != i + 2) { // c = 2
            cout << "WRONG array value at " << i << ": " << __array__[i] << endl;
            e = EXIT_FAILURE;
        }
    }

    exit(e);
}

