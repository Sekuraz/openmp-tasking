
#include "omp.h"

#define AS 10000

void test(int a[AS], int* p) {
    for(int i = 0; i < AS; i++) {
        std::cout << "generating task " << i << std::endl;
        #pragma omp task untied mergeable if(i == 3) final(i == 5) depend(in: a)
        {
            a[i] = i + *p;
        }
    }
}

int main(int argc, char** argv) {

    std::cout << "hello from main" << std::endl;

    int* a = new int[AS];
    int b[AS];
    int c = 0;
    int* p = &c;

    #pragma omp parallel
    #pragma omp single
    /*
    for(int i = 0; i < AS; i++) {
        #pragma omp task untied mergeable if(i == 3) final(i == 5)
        {
            a[i] = i + *p;
        }
    } */
    test(a, p);
    return a[0];
}

