#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "ProcessInfo.h"
#include "procstat.h"


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int time_slice[5] = {1,2,4,8,16};

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

int
setpriority(int pid,int priority){
  struct proc *p;
  int old_priority = 0;
  acquire(&ptable.lock);

  for(p = ptable.proc;p < &ptable.proc[NPROC];p++){
    if(p->pid == pid){
      old_priority = p->priority;
      p->priority = priority;
      break;
    }
  }

  release(&ptable.lock);

  return old_priority;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  
  //c0++;
  //q0[c0] = p;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  #ifdef PBS
    p->priority = 60;
  #else
  #ifdef MLFQ
    p->priority = 0;
  #endif
  #endif
  p->ctime = ticks;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  p->etime = 0;
  p->iotime = 0;
  p->rtime = 0;
  p->num_of_execution = 1;
  p->lastActualWorking = ticks;
  #ifdef MLFQ
  p->ticks[0] = p->ticks[1] = p->ticks[2] = p->ticks[3] = p->ticks[4] = 0;
  p->lastScheduled = ticks;
  #endif
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  p->ctime = ticks;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  curproc->etime = ticks;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int
waitx(int *wtime,int *rtime)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        *wtime = p->etime - p->ctime - p->rtime;
        *rtime = p->rtime;
        //cprintf("Waiting time : %d Turn Around Time : %d\n",*wtime,p->etime);
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifdef PBS
void
scheduler(void)
{
  struct proc *p,*p1;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();
    struct proc *high_priority = 0;
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){ 
          if(p->state != RUNNABLE){
              continue;
          }

          high_priority = p;

          for(p1 = ptable.proc;p1 < &ptable.proc[NPROC];p1++){
              if((p1->state == RUNNABLE) && (high_priority->priority > p1->priority))
                high_priority = p1;
          }

          if(high_priority->priority > p1->priority)
            high_priority = p1;

          p = high_priority;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.

      if(p > 0){
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
    }
    release(&ptable.lock);

  }
}
#else
#ifdef MLFQ
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  //int j;
  int lastExecuted = 0;
  struct proc *exc;

  for(;;) {
    sti();
    acquire(&ptable.lock);
    struct proc *processes_found[5] = {0};

    int i;

    for(p = ptable.proc;p < &ptable.proc[NPROC];p++){
      if(p->pid == lastExecuted || p->state != RUNNABLE)
      continue;

      if(p->priority > 0 && (ticks-(p->lastActualWorking)) > 250 && p->pid > 2){
        p->priority--;
        p->lastActualWorking = ticks;
        cprintf("Procees %d aged and new queue is %d\n",p->pid,p->priority);
        break;
      }
    }
 
    for(p = ptable.proc;p < &ptable.proc[NPROC];p++){
      
      if(p->state != RUNNABLE){
        continue;
      }
      
        if(p->priority != 4){
          if(processes_found[p->priority] == 0){
            processes_found[p->priority] = p;
          } 

          else if(p->ctime < processes_found[p->priority]->ctime && processes_found[p->priority]->state == RUNNABLE){
            processes_found[p->priority] = p;
          }
        }

        else{
          if(processes_found[p->priority] == 0){
            processes_found[p->priority] = p;
          } 

          else if(processes_found[p->priority]->lastScheduled > p->lastScheduled && processes_found[p->priority]->state == RUNNABLE){
            processes_found[p->priority] = p;
          }
        }  
    }

    for(i = 0;i < 5;i++){
      if(processes_found[i] != 0){

        if(processes_found[i]->pid > 2){
          int flag = 0;
          while(processes_found[i]->state == RUNNABLE){
              exc = processes_found[i];
              c->proc = exc;
              exc->ticks[i]++;
              lastExecuted = processes_found[i]->pid;
              exc->lastActualWorking = ticks;
              //exc->lastScheduled = ticks;
              switchuvm(exc);
              exc->state = RUNNING;
              //cprintf("%d ",c->proc->pid);
              swtch(&(c->scheduler), exc->context);
              switchkvm();
              c->proc = 0;

              if(exc->ticks[i]%time_slice[i] == 0 && exc->ticks[i] > time_slice[i])
              break;

              for(p = ptable.proc;p < &ptable.proc[NPROC];p++){
                if(p->state == RUNNABLE && p->priority < processes_found[i]->priority){
                  flag = 1;
                  break;
                }
              }
              if(flag == 1)
              break;   
          }

          if(processes_found[i]->ticks[i]%time_slice[i] == 0 && processes_found[i]->ticks[i] >= time_slice[i] && i != 4){
                processes_found[i]->priority++;
                processes_found[i]->lastScheduled = ticks;
                processes_found[i]->lastActualWorking = ticks;
              }

          if(i == 4){
            if(processes_found[i]->ticks[i]%time_slice[i] == 0 && processes_found[i]->ticks[i] >= time_slice[i]){
              processes_found[i]->lastScheduled = ticks;
              processes_found[i]->lastActualWorking = ticks;
            }
          }
          processes_found[i]->num_of_execution++;
          break;
        }

        else {
          int flag = 0;
          while(processes_found[i]->state == RUNNABLE){

            exc = processes_found[i];
            c->proc = exc;
            exc->lastActualWorking = ticks;
            //exc->ticks[i]++;
              //exc->lastScheduled = ticks;
            switchuvm(exc);
            lastExecuted = processes_found[i]->pid;
            //cprintf("%d ",c->proc->pid);
            exc->state = RUNNING;
            swtch(&(c->scheduler), exc->context);
            switchkvm();
            c->proc = 0;

            for(p = ptable.proc;p < &ptable.proc[NPROC];p++){
              if(p->state == RUNNABLE && p->priority < processes_found[i]->priority){
                  flag = 1;
                  break;
              }
            }

            if(processes_found[i]->state == SLEEPING)
            break;

            if(flag == 1)
            break;
          }
          processes_found[i]->num_of_execution++;
          break;
        }
      }
    }

    release(&ptable.lock);
  }
}
#else
#ifdef FCFS
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    struct proc *minp = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        
        if(p->state != RUNNABLE)
          continue;

        if(minp == 0)
          minp = p;
        else if(p->ctime < minp->ctime){
          minp = p;
        }
    }
    //cprintf("Switching from %d\n",c->proc->pid);
    if(minp != 0){
      //cprintf("Switching from ");
      c->proc = minp;
      switchuvm(minp);
      minp->state = RUNNING;
      //minp->rtime = ticks;
      swtch(&(c->scheduler),c->proc->context);
      switchkvm();
    }
    release(&ptable.lock);

  }

}
#else
#ifdef DEFAULT
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

     
      if(p->state != RUNNABLE)
        continue;
     

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      //cprintf("Process %d with pid running\n", p->pid);
      if(p > 0){
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        //cprintf("Process %s with pid %d running\n", p->name, p->pid);
        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
    }
    release(&ptable.lock);

  }
}
#endif
#endif
#endif
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf(" %d",p->priority);
    cprintf(" %d %d %d %d %d",p->ticks[0],p->ticks[1],p->ticks[2],p->ticks[3],p->ticks[4]);
    cprintf("\n");
  }
}
struct proc * getptable_proc(void){
  return ptable.proc;
}

int 
getprocs(struct ProcessInfo* processInfoTable){
  struct proc *p;  
  int count = 0;
  int i;
  for (i = 0, p = ptable.proc; p < &ptable.proc[NPROC] && i < NPROC; i++,p++)
  {
    if(p->state == UNUSED)
    {
      continue;
    }
    count++;
    processInfoTable[i].pid = p->pid;
    if(i == 0){
        processInfoTable[i].ppid = -1;
    }
    else{
         processInfoTable[i].ppid = p->parent->pid;
    }
   
    processInfoTable[i].state = p->state;
    processInfoTable[i].sz = p->sz;

    for (int j = 0; j < 16; j++)
    {
       processInfoTable[i].name[j] = p->name[j];
    }
    processInfoTable[i].priority = p->priority;
  }
  p = 0;
  return count;

}

int getpinfo(int *pid,struct proc_stat *status) {
  
  struct proc *p;
  int found = 0;
  for(p = ptable.proc;p < &ptable.proc[NPROC];p++){
    if((p->pid) == *pid){
      found  = 1;
      status->pid = p->pid;
      //cprintf("%d   ",status->pid);
      status->runtime = p->rtime;
      //cprintf("%d  ",status->runtime);
      status->num_run = p->num_of_execution;
      status->current_queue = p->priority;
      for(int i = 0;i < 5;i++){
        status->ticks[i] = p->ticks[i];
      }
      break;
    }
  }

  if(found == 0){
    cprintf("No process exist with given pid");
  }

  return 0;
}