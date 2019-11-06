#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc,char* argv[]){
    if(argc < 3 || argc > 3){
        printf(1,"Please enter required number of arguments only\n");
    }

    else{
        int x = atoi(argv[1]);
        int y = atoi(argv[2]);
        setpriority(x,y);
    }
    exit();
    return 0;
}