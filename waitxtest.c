#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"



int main(){
    int x;
    for(float j = 0;j < 100000;j+=0.005)
        x = x+3.14*3.43;

    printf(1,"Execution Completed %d\n");
    exit();
}



