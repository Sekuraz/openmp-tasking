//
// Created by markus on 07.11.18.
//

#ifndef PROJECT_RECEIVER_H
#define PROJECT_RECEIVER_H

#include <mpi.h>
#include <vector>
#include <mutex>

#include "constants.h"
#include "utils.h"


class Receiver {
public:
    explicit Receiver(int node_id);

    Message receive_message();

//protected:
    int node_id;
    std::mutex mpi_receive_lock; // other threads which are using mpi have to block


private:
    int receiving = false;
    std::vector<int> buffer;
    MPI_Request current_request;
};


#endif //PROJECT_RECEIVER_H
