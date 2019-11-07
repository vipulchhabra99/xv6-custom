#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int childPids [10];
int ranks[10];


int main(int argc, char *argv[]){

    int a, b,status;
    for(int i = 0;i < 10;i++) {

        if((childPids[i] = fork()) > 0){
            //status = waitx(&a,&b);
            //printf(1, "Wait Time = %d\n Run Time = %d \nStatus %d \n", a, b, status);
            continue;
        }

        else{
            int x;
            for(float j = 0;j < 32000;j+=0.001)
            x = x+3.14*3.43;

            x = i;
            printf(1,"I am completed my execution %d\n",x);

            exit();
        }
    }

    for(int i = 0;i < 10;i++){
        setpriority(childPids[i],60-(5*i));

    }
    for(int i = 0;i < 10;i++){
        status = waitx(&a, &b);
		printf(1, "Wait Time = %d\n Run Time = %d \nStatus %d \n", a, b, status);
    }

    exit();
}