//
// Created by markus on 05.11.18.
//

#ifndef LIBTDOMP_CONSTANTS_H
#define LIBTDOMP_CONSTANTS_H

static int worker_per_runtime = 100;

enum TAG {
    NO_MESSAGE,
    CREATE_TASK,
    FINISH_TASK,
    RUN_TASK,
    REQUEST_MEMORY,
    WRITE_MEMORY,
    REPORT_CAPACITY,
    SHUTDOWN
};

#endif //LIBTDOMP_CONSTANTS_H
