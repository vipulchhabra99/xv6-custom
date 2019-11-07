#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "procstat.h"

int childPids [10];
int ranks[10];


int main(int argc, char *argv[]){

    //struct proc_stat stats;
    for(int i = 0;i < 10;i++) {

        if((childPids[i] = fork()) > 0){
            for(int j = 0;j < 100;j++)
            {  
                setpriority(childPids[0],0);
            }

            if(i == 1){
                for(int j = 0;j < 100;j++)
                {  
                    setpriority(childPids[1],1);
                }
            }

            continue;
        }

        else{
            int x;
            for(float j = 0;j < 10000;j+=0.005)
            x = x+3.14*3.43;

            x = i;
            printf(1,"I am completed my execution %d\n",x);
            exit();
        }
    }

    

    
    exit();
}