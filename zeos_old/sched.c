/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */

//canviat if 0 a if 1
#if 1
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

extern struct list_head blocked;
struct list_head freequeue;
struct list_head readyqueue;
struct task_struct *idle_task;
int current_max_pid = 1;
int current_quantum;


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}

int get_next_pid(){
  int ret = current_max_pid;
  current_max_pid++;
  return ret;
}

int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_idle (void)
{
/*x 1) Get an available task_union from the freequeue to contain the characteristics of this process
  x 2) Assign PID 0 to the process.
  x 3) Initialize field dir_pages_baseAaddr with a new directory to store the process address space
  using the allocate_DIR routine.
  ? 4) Initialize an execution context for the procees to restore it when it gets assigned the cpu
  (see section 4.5) and executes cpu_idle.
  x 5) Define a global variable idle_task ->   struct task_struct * idle_task;
  ? 6) Initialize the global variable idle_task, which will help to get easily the task_struct of the
  idle process.
*/
  //1)
  struct list_head *fq = list_first(&freequeue);
  idle_task = list_head_to_task_struct(fq);   
  list_del(fq); 
  //2)
  idle_task->PID = 0;
  //3)
  allocate_DIR(idle_task);
  //4)
  unsigned long *idletask_stack = ((union task_union *)idle_task)->stack;

  idletask_stack[KERNEL_STACK_SIZE - 1] = (unsigned long)&cpu_idle;
  idletask_stack[KERNEL_STACK_SIZE - 2] = (unsigned long)0;  
  //desar stack a union idle_task? punter?
}

void init_task1(void)
{
/*x 1) Get an available task_union from the freequeue to contain the characteristics of this process
  x 2) Assign PID 1 to the process.
  x 3) Initialize field dir_pages_baseAaddr with a new directory to store the process address space
  using the allocate_DIR routine.
  X 4) Complete the initialization of its address space, by using the function set_user_pages (see file
  mm.c). This function allocates physical pages to hold the user address space (both code and
  data pages) and adds to the page table the logical-to-physical translation for these pages.
  Remind that the region that supports the kernel address space is already configure for all
  the possible processes by the function init_mm.
  ? 5) Update the TSS to make it point to the new_task system stack.
  X 6) Set its page directory as the current page directory in the system, by using the set_cr3
  function (see file mm.c)..
*/
  //1)
  struct list_head *fq = list_first(&freequeue);
  struct task_struct *task1_task = list_head_to_task_struct(fq);
  list_del(fq); 
  //2)
  task1_task->PID = 1;
  //3)
  allocate_DIR(task1_task);
  //4)
  set_user_pages(task1_task);
  //5) ?
  tss.esp0 = KERNEL_ESP((union task_union *)&task1_task);
  //6)
  set_cr3(get_DIR(task1_task));


  // afegir a cua ready?
  list_add_tail(&task1_task->list, &readyqueue);

}

void init_freequeue (void) {
  INIT_LIST_HEAD(&freequeue);

  int i;
  for (i = 0; i < NR_TASKS; ++i) {
    list_add_tail(&task[i].task.list,&freequeue);
  }
}

void init_readyqueue (void) {
  INIT_LIST_HEAD(&readyqueue);
}

int get_quantum (struct task_struct *t){
  return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum){
  t->quantum = new_quantum;
}

void init_sched(){

}

void schedule(){
  update_sched_data_rr();
  if (needs_sched_rr()) {
    update_process_state_rr(current(), &readyqueue);
    sched_next_rr();
  }
}

void update_sched_data_rr(){
  --current_quantum;
}

int needs_sched_rr(){
  /*returns: 1 if it is necessary to change the current process and 0
      otherwise
  */
  if(current_quantum == 0){
    if(!list_empty(&readyqueue)) return 1;
    else{
      int quant = DEFAULT_QUANTUM > get_quantum(current()) ? get_quantum(current()) : DEFAULT_QUANTUM;
      current_quantum = quant;
    }
  } 
  return 0;
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue){
  if (dst_queue == &readyqueue) t->state = ST_READY;
  else if(dst_queue == NULL) t->state = ST_RUN;
  else t->state = ST_BLOCKED;

  set_quantum(t, DEFAULT_QUANTUM);

  /* Remove process t from its current queue and put it to dst_queue only
   * if that process is not the idle and it's not the only available whith
   * ready status.
   */
  if ((t != idle_task) & (!list_empty(&readyqueue))) {
    list_del(&(t->list));
    list_add_tail(&(t->list), dst_queue);
  }
}

void sched_next_rr(){
  struct task_struct* next_task;
  // If no more processes in the readyqueue set next process to idle.
  if (list_empty(&readyqueue))
    next_task = idle_task;
  else 
    next_task = list_head_to_task_struct(list_first(&readyqueue));

  next_task->state = ST_RUN;

  // Sets system quantum to the next process one.
  current_quantum = get_quantum(next_task);

  // Only switch context if next_task is not the current one.
  if (next_task != current())
    task_switch((union task_union*) next_task);
}

void inner_task_switch(union task_union *n){
  /*
  1) Update the TSS to make it point to the new_task system stack.
  2) Change the user address space by updating the current page directory: use the set_cr3
    funtion to set the cr3 register to point to the page directory of the new_task.
  3) Store the current value of the EBP register in the PCB. EBP has the address of the current
    system stack where the inner_task_switch routine begins (the dynamic link).
  4) Change the current system stack by setting ESP register to point to the stored value in the
    new PCB.
  5) Restore the EBP register from the stack.
  6) Return to the routine that called this one using the instruction RET.
  */

  //1)
  tss.esp0 = n->stack[KERNEL_STACK_SIZE];
  //2)
  set_cr3(get_DIR(&(n->task)));
  //3) CAL FER PUSH EBP?
  __asm__ __volatile__(
    "pushl %%ebp\n\t" 
    "movl %%ebp,%0\n\t"
    :"=g"(current()->kernel_esp) 
    : : 
  ); 
  //4), 5), 6)
  __asm__ __volatile__ (
      "movl %0, %%esp\n\t"
      "popl %%ebp\n\t"
      "ret\n\t"
      : :"g" (n->task.kernel_esp)
      );

}

void task_switch(union task_union *n){
/*This routine
  1) saves the registers ESI, EDI and EBX 16 , 
  2) calls the inner_task_switch routine, and
  3) restores the previously saved registers.
*/
  __asm__ (
    "pushl %%esi\n\t"
    "pushl %%edi\n\t"
    "pushl %%ebx\n\t"
    : : :
  );

  inner_task_switch(n);

  __asm__ (
    "popl %%ebx\n\t"
    "popl %%edi\n\t"
    "popl %%esi\n\t"
    : : :
  );
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

