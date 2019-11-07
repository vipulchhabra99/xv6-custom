#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[])
{
    int k, n, id;
    int a,b;
    double x=0, z;

    if(argc < 2)
    n = 2;

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
            printf(1, "Child %d created \n", id);
            waitx(&a,&b);
        }
        else
        {   // Child
            printf(1, "Child %d created\n", getpid());
            for(z=0;z<8000000.0;z+=0.01)
                x = x + 3.3454*245.23; // Useless calculations to consume CPU time
            break;
        }
    }
    exit();
}