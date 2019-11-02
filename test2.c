#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(){
    printf(1,"Priority Based Scheduler Testing\n");
    
    if(fork() == 0){
        int x;
        for(int i = 0;i < 10000000;i++){
            x = 3.14*89.23;
        }
        return x;
    }

    if(fork() == 0){
        float x;
        for(int i = 0;i < 10000000;i++){
            x = 3.14*89.23;
        }
        //printf(1,"%f",x);
        return x;
    }
}