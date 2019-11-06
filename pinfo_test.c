#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "procstat.h"

int main(int argc, char *argv[])
{
    if(argc < 2 || argc > 2){
        printf(1,"only one argument required\n");
    }

    else{
        struct proc_stat proc;
        int pid = atoi(argv[1]);
        getpinfo(&pid,&proc);
        //printf(1,"%d\n",atoi(argv[1]));
        /*printf(1,"******DETAILS OF THE PROCESS************\n");
        printf(1,"Pid : %d\n",proc.pid);
        printf(1,"Rntime : %d\n",proc.runtime);
        printf(1,"Number_of_time_scheduled : %d\n",proc.num_run);
        printf(1,"Current Queue : %d\n",proc.current_queue);
        printf(1,"Ticks Recieved In Each Queue : %d %d %d %d %d\n",proc.ticks[0],proc.ticks[1],proc.ticks[2],proc.ticks[3],proc.ticks[4]);*/
        exit();
    }   

    return 0;
}