
#include "omp.h"

#include <thread>
#include <chrono>

#define AS 100

void test(int a[AS], int* p) {
    for(int i = 0; i < AS; i++) {
        std::cout << "generating task " << i << std::endl;
        #pragma omp task untied mergeable if(i == 3) final(i == 5) depend(in: a)
        {
            a[i] = i + *p;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

int main(int argc, char** argv) {

    std::cout << "hello from main" << std::endl;

    int* a = new int[AS];
    int b[AS];
    int c = 0;
    int* p = &c;

    test(a, p);
    return a[0];
}

