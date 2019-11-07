#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(){
    printf(1,"Priority Based Scheduler Testing\n");

    int x,a,b;

    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }
    
    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }

    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }
    
    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }

    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }
    
    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }

    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }
    
    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }

    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }
    
    if(fork() == 0){
        for(float z=0;z<100000.0;z+=0.01)
		x = x + 3.14*69.69;
    }
    
    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);
    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);

    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);
    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);

    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);
    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);

    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);
    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);

    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);
    waitx(&a,&b);
    printf(1,"%d %d\n",a,b);

    exit();
}