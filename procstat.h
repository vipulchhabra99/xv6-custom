#ifndef _PROCSTAT_H_
#define _PROCSTAT_H_
#include "types.h"

struct proc_stat{
	int pid;
	int runtime;
    int num_run;
    int current_queue;
    int ticks[5];
};
#endif