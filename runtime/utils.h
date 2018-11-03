#ifndef TDOMP_UTIL_H
#define TDOMP_UTIL_H

// This file contains constants and types used by tdomp

const int TAG_MISC = 0;
const int TAG_CREATE = 10;
const int TAG_COMMAND = 20;
const int TAG_SHUTDOWN = 30;
const int TAG_WAIT = 40;
const int TAG_WAITTASK = 50;
const int TAG_WAITALL = 60;
const int TAG_WAKEUPTASK = 70;
const int TAG_FREECAPACITY = 80;
const int TAG_MEMORY_TRANSFER = 90;


const int re_rank = 0;

using node_id_t = int;
using task_id_t = int;
using code_id_t = int;

#endif
