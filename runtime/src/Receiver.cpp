//
// Created by markus on 07.11.18.
//

#include <iomanip>

#include "Receiver.h"
#include "utils.h"


using namespace std;

Receiver::Receiver(int node_id) : node_id(node_id){

}

Message Receiver::receive_message() {
    int tag, source;
    MPI_Status status;

    int receive_pending = false;
    int receive_finished = false;

    if (!receiving) {

        //printf("runtime %d recv is not running. probing...\n", node_id);
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &receive_pending, &status);
    }
    if (receive_pending) {
        this->mpi_receive_lock.lock();
        int buffer_size;
        //printf("runtime %d started receiving\n", node_id);
        MPI_Get_count(&status, MPI_INT, &buffer_size);

        tag = status.MPI_TAG;
        source = status.MPI_SOURCE;

        buffer = vector<int>((size_t)buffer_size, 0);

//        cout << setw(6) << node_id << ": receiving " << setw(10) << buffer.size() << " bytes from "
//                << setw(6) << source << " with tag " << tag << endl;
        MPI_Irecv(&buffer[0], buffer_size, MPI_INT, source, tag, MPI_COMM_WORLD, &current_request);

        receiving = true;
    }
    if (receiving) {
        MPI_Test(&current_request, &receive_finished, &status);
        if (receive_finished) {
            receiving = false;
            this->mpi_receive_lock.unlock();

            tag = status.MPI_TAG;
            source = status.MPI_SOURCE;

//            cout << setw(6) << node_id << ": received  " << setw(10) << buffer.size() << " bytes from "
//                    << setw(6) << source << " with tag " << tag << endl;

            auto tag_t = static_cast<TAG>(tag);

            Message m;
            m.tag = tag_t;
            m.source = source;
            m.data = buffer;
            return m;
        }
    }

    Message m;
    m.tag = TAG::NO_MESSAGE;
    m.source = -1;
    m.data = vector<int>(0);
    return m;
}
