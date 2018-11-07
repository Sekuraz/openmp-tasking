
#include <iostream>

#define AS 10000

int main(int argc, char** argv) {

    std::cout << "hello from main" << std::endl;

    int* a = new int[AS];
    int b[AS];
    int c = 0;
    int* p = &c;

    for(int i = 0; i < AS; i++) {
        a[i] = i + *p;
    }
    std::cout << (a[23] == 23 ? "SUCCESS" : "FAILURE");
    return;
}

