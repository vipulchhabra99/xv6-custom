#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int k, n, id;
    double x=0, z;

    if(argc < 2)
    n = 80;

    else
    n = atoi(argv[1]);

    x = 0;
    id = 0;
    for(k=0; k<n; k++)
    {
        id = fork();
        if(id < 0)
            printf(1, "%d failed in fork!\n", getpid());
        else if(id > 0)
        {   
            wait();
        }
        else
        {   
            for(z=0;z<10000.0;z+=0.01)
                x = x + 34.14*24.13; 
            break;
        }
    }
    exit();
}