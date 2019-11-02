#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "ProcessInfo.h"


int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_waitx(void)
{
  int *wtime;
  int *rtime;
  
  if(argptr(0,(char**)&wtime,sizeof(int)) < 0){
    return 12;
  }

  else if(argptr(1,(char**)&rtime,sizeof(int)) < 0){
    return 13;
  }

  else{
    return waitx(wtime,rtime);
  }
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int 
sys_getprocs(void){
  struct ProcessInfo *processInfoTable;
  if(argptr(0,(char**)&processInfoTable,sizeof(struct ProcessInfo) * NPROC) < 0) {
    return -1;
  }
  return getprocs(processInfoTable);

}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

extern int setpriority(int,int);

int
sys_setpriority(void){
  int pid, pr;
  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &pr) < 0)
    return -1;

  if(pr < 0 || pr > 100){
    cprintf("Invalid Priority range\n");
    return -1;
  }

  return setpriority(pid, pr);
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

extern struct proc * getptable_proc(void);

int sys_getptable(void){
  int size;
  char *buf;
  char *s;
  struct proc *p = '\0';
  
  if (argint(0, &size) <0){
    return -1;
  }
  if (argptr(1, &buf,size) <0){
    return -1;
  }
  
  s = buf;
  p = getptable_proc();
  
  while(buf + size > s && p->state != UNUSED){
    *(int *)s = p->state;
    s+=4;
    *(int *)s = p -> pid;
    s+=4;
    *(int *)s = p->parent->pid;
    s+=4;
    memmove(s,p->name,16);
    s+=16;
    p++;
  } 
  return 0;
}