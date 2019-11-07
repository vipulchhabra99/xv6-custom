#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define TRUE 1
#define FALSE 0
#define TH 100

/*int to_be_or_not(int to_be) {
  int count = 0;

  while(count++ < TH) {
    if (to_be) printf(1, "to be!\n");
    if (!to_be) printf(1, "not to be!\n");
    //sleep(1);
  }
  return 0;
}
    
int main(int argc, char **argv) {
  // first child   
  if (fork() == 0) {
    to_be_or_not(TRUE);
    exit();
  }
      
  // second child
  if(fork() == 0) {
    to_be_or_not(FALSE);
    exit();
  }

  // parent
  wait();
  wait();
  exit();
}*/

/*int main(){
  sleep(3);
  return 0;
}*/

#define PROCESSES 10

int ranks[PROCESSES];
int pids [PROCESSES];

int findchild(int childPid){
    int curPid = 0;
    int i = 0;
    while(i < PROCESSES){
        curPid = pids[i];
        if (curPid == childPid)
            return i;

        i++;
    }
    return -1;
}

void status_check(){
    int childReturn = -1;
    int a,b,i = 0;
    while(i < PROCESSES){
        while((childReturn = waitx(&a,&b)) < 0);
        int childIndex = findchild(childReturn);
        ranks[i] = childIndex;
        i++;
    }
}



int main(int argc, char* argv[])
{

    for(int i = 0; i < PROCESSES;i++){
        if((pids[i] = fork())> 0){
            //setpriority(pids[i],(50+PROCESSES) - 2*i); // To set the priorites in case of priority scheduling
            if(i == PROCESSES -1){
                status_check();
                printf(1,"TEST RESULTS\n");
                printf(1,"========================\n");
                for(int i = 0; i < PROCESSES; i++){
                    printf(1,"Child: %d Rank: %d\n",ranks[i], i);
                }
                printf(1,"========================\n");
            }
            continue;
        }else{
            
            for (int j = 0; j < 100000; j++){
                if(j%1000 == 0)
                    printf(0, "Child number %d executing\n",i);

                for(float j = 0;j < 10000; j += 0.0001); 
            }
            printf(2,"Child %d finished execution\n", i);
            break;
        }
    }    
    exit();
}





/*int main()
{     
    if(fork() == 0) {
            //setpriority(pid[0],62);
            for (int j = 0; j < 10000; j++){
                if(j%1000 == 0)
                    printf(0, "childNum: %d round: %d\n",0, j);
            }
    }

    if(fork() == 0){
        for(int i = 0; i  < 1000;i++){
            printf(1,"Testing in progress \n");
        }
    }

    for(int i = 0;i < 2;i++){
        wait();
    }

    return 0;
}*/