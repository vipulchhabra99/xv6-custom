#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int main (int argc,char *argv[])
{

	int pid;
	int a, b;
	int status = 0;	
	pid = fork();
	if (pid == 0)
  	{	
  		exec(argv[1], argv);
    }
  	else
 	{
    	status = waitx(&a, &b);
		printf(1, "Wait Time = %d\n Run Time = %d \nStatus %d \n", a, b, status); 
 	}  
 	
 	exit();
}