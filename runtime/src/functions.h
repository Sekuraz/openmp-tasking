//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_FUNCTIONS_H
#define LIBTDOMP_FUNCTIONS_H

#include <fstream>
#include <sstream>
#include <iostream>
#include <sys/resource.h>
#include <malloc.h>


// get the number of bytes which should be transmitted to the other node for the given pointer
size_t get_allocated_size(void* pointer) {
    auto t = (size_t) pointer;

    std::ifstream maps("/proc/self/maps");
    bool isHeap = false;
    std::string heap("heap"), stack("stack");

    size_t start_of_heap = 0;

    for (std::string line; std::getline(maps, line); ) {
        size_t start, end;

        // this char reads the '-' between the 2 values in the file, otherwise the return value is incorrect
        char discard;

        std::istringstream iss(line);
        iss >> std::hex >> start >> discard >> end;

        if (start < t && t < end) {

            if (line.find(heap) != std::string::npos) {
                isHeap = true;
                start_of_heap = start;
                break;
            }
            else if (line.find(stack) != std::string::npos) {
                // return the maximum stack size, this should be low enough to not be that much of a burden
                // TODO look wether the relevant memory is already transmitted
                rlimit lim;
                getrlimit(RLIMIT_STACK, &lim);
                return lim.rlim_cur;
            }
            else {
                // maybe static data from a library or the binary itself

                return 20;

                // TODO get the size of the variable by parsing the elf headers
                // SEE https://stackoverflow.com/questions/45697799/determine-total-size-of-static-variables-of-class
            }

        }
    }

    if (!isHeap) {
        throw std::domain_error("unable to determine the size of a pointee");
    }

    size_t loops = 0;
    size_t size = 0;
    do {
        size = malloc_usable_size((void*)t);
        t--;
        loops++;
        if ((size_t) pointer <= start_of_heap) {
            throw std::domain_error("unable to determine the size of a pointee");
        }
    } while (size == 0);

    return size - loops;
}
#endif //PROJECT_FUNCTIONS_H
