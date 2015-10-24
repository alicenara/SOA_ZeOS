/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>
 
#include <variables.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork()
{
  return 0;
}

int sys_fork()
{
  /*
  a) Get a free task_struct for the process. If there is no space for a new process, an error
    will be returned.
  b) Inherit system data: copy the parent’s task_union to the child. Determine whether it is
    necessary to modify the page table of the parent to access the child’s system data. The
    copy_data function can be used to copy.
  c) Initialize field dir_pages_baseAddr with a new directory to store the process address
    space using the allocate_DIR routine.
  d) Search physical pages in which to map logical pages for data+stack of the child process
    (using the alloc_frame function). If there is no enough free pages, an error will be
    return.
  e) Inherit user data:
    i) Create new address space: Access page table of the child process through the directory 
      field in the task_struct to initialize it (get_PT routine can be used):
      A) Page table entries for the system code and data and for the user code can be a
        copy of the page table entries of the parent process (they will be shared)
      B) Page table entries for the user data+stack have to point to new allocated pages
        which hold this region
    ii) Copy the user data+stack pages from the parent process to the child process. The
      child’s physical pages cannot be directly accessed because they are not mapped in
      the parent’s page table. In addition, they cannot be mapped directly because the
      logical parent process pages are the same. They must therefore be mapped in new
      entries of the page table temporally (only for the copy). Thus, both pages can be
      accessed simultaneously as follows:
      A) Use temporal free entries on the page table of the parent. Use the set_ss_pag and
        del_ss_pag functions.
      B) Copy data+stack pages.
      C) Free temporal entries in the page table and flush the TLB to really disable the
        parent process to access the child pages.
  f) Assign a new PID to the process. The PID must be different from its position in the
    task_array table.
  g) Initialize the fields of the task_struct that are not common to the child.
  h) Think about the register or registers that will not be common in the returning of the
    child process and modify its content in the system stack so that each one receive its
    values when the context is restored.
  i) Prepare the child stack with a content that emulates the result of a call to task_switch.
    This process will be executed at some point by issuing a call to task_switch. Therefore
    the child stack must have the same content as the task_switch expects to find, so it will
    be able to restore its context in a known position. The stack of this new process must
    be forged so it can be restored at some point in the future by a task_switch. In fact this
    new process has to a) restore its hardware context and b) continue the execution of the
    user process, so you must create a routine ret_from_fork which does exactly this. And
    use it as the restore point like in the idle process initialization 4.4.52
  j) Insert the new process into the ready list: readyqueue. This list will contain all processes
    that are ready to execute but there is no processor to run them.
  k) Return the pid of the child process.
  */
  //vars que necessitarem a partir de d

  int pagines_data[NUM_PAG_DATA];
  int i = 0;

  //a)
  int noMoarPCB = list_empty(&freequeue);
  if(noMoarPCB) return -2; //error de que no hi ha pcb lliure
  
  struct list_head *fq = list_first(&freequeue);
  struct task_struct *child_task = list_head_to_task_struct(fq);
  list_del(fq);
  //b)
  struct task_struct *parent_task = current();
  union task_union *parent_union = (union task_union *) parent_task;
  union task_union *child_union = (union task_union *) child_task;
  copy_data(parent_union, child_union, sizeof(union task_union));
  /*b) part de "Determine whether it is necessary to modify the page table of the parent 
  to access the child’s system data."
  Solucio: wat
  */
  //c)
  allocate_DIR(child_task);

  //d) data+stack = pages data
  pagines_data[0] = alloc_frame();
  while(i < NUM_PAG_DATA && pagines_data[i] != -1){
    i++;
    pagines_data[i] = alloc_frame();
  }
  //si no hi ha prous pàgines, a més d'error s'han d'alliberar les que s'havien agafat
  if(pagines_data[i] == -1){
    for (i = i; i>=0; i--){
      free_frame(pagines_data[i]);
    } 
    //es torna a deixar lliure el pcb
    list_add_tail(&(child_task->list), &freequeue);
    return -3; //error de que no hi ha pagina lliure
  }

  //e) 
  //  i)
  page_table_entry* pag_child = get_PT(child_task);
  page_table_entry* pag_parent = get_PT(parent_task);
  //    A)system code and user data es shared
  for (i = PAG_LOG_INIT_CODE; i < PAG_LOG_INIT_DATA; i++) {
    set_ss_pag(pag_child, i, get_frame(pag_parent, i));
  }
  //    B) user data + stack no
  for (i = 0; i < NUM_PAG_DATA; i++) {
    set_ss_pag(pag_child, PAG_LOG_INIT_DATA+i, pagines_data[i]);

 //  ii)
  //    A) creació d'espai a la part del pare per copiar dades i passarles al fill
    unsigned int logic_addr = (i + PAG_LOG_INIT_DATA) * PAGE_SIZE;
    set_ss_pag(pag_parent, i + PAG_LOG_INIT_DATA + NUM_PAG_DATA, pagines_data[i]);
  //    B) còpia de les dades
    copy_data((void *)(logic_addr), (void *)(logic_addr + (PAGE_SIZE * NUM_PAG_DATA)), PAGE_SIZE);
  //    C)    esborrat de les dades
    del_ss_pag(pag_parent, i + PAG_LOG_INIT_DATA + NUM_PAG_DATA);
  }
  //    C) bis -> esborrat de tlb
  set_cr3(get_DIR(parent_task));
  //f)
  child_task->PID = get_next_pid();
  //g)
  child_task->state = ST_READY;
  child_task->quantum = DEFAULT_QUANTUM;
  //h)
   unsigned int ebp;
  __asm__ __volatile__(
      "mov %%ebp,%0\n"
      :"=g"(ebp)
  );
  //i)
  unsigned int stack_stride = (ebp - (parent_task->PID))/sizeof(unsigned long);

  child_union->stack[stack_stride-1] = 0; //ebp
  child_union->stack[stack_stride] = (unsigned long)&ret_from_fork;
  child_union->task.kernel_esp = (unsigned long)&child_union->stack[stack_stride-1];
  //j)
  list_add_tail(&child_task->list, &readyqueue);
  //k)
  return child_task->PID;
}

void sys_exit()
{  
}

int sys_write(int fd, char * buffer, int size){
  //fd: file descriptor. In this delivery it must always be 1.
  //buffer: pointer to the bytes.
  //size: number of bytes.
  //return ’ Negative number in case of error (specifying the kind of error) and
  //the number of bytes written if OK.

  int ret = 0;
  int myMaxBuff = 1024;
  int sizeOriginal = size;

  ret = check_fd(fd,ESCRIPTURA);
  if(!ret && buffer == NULL) ret = -1; //buscar error
  if(!ret && size >= 0){


    char mybuff[myMaxBuff];
    int numbytes = 0;
    
    while (size >= myMaxBuff || size == -1){
      int comprovacio = copy_from_user(buffer, mybuff, size);
      if(comprovacio){ 
        ret = comprovacio; //error, buscar codi error;
        size = -1;
      } else{
        int sizeEscrit = sys_write_console(mybuff, size);

        if(sizeEscrit < 0){
          ret = sizeEscrit; //error, buscar codi error;
          size = -1;
        }else{
          numbytes += sizeEscrit;
          buffer += myMaxBuff;
          size -= myMaxBuff;
        }         
      }      
    }
    if(size > 0){
      // volta final
      int comprovacio = copy_from_user(buffer, mybuff, size);
      if(comprovacio) ret = comprovacio; //error, buscar codi error;
      else{
        int sizeEscrit = sys_write_console(mybuff, size);
        numbytes += sizeEscrit; //error, buscar codi error;
      }

      if(ret == 0){
        if (numbytes != sizeOriginal && ret == 0) ret = -1; //buscar codi error
        else ret = numbytes;
      }      
    }
    
  }else ret = -1; //buscar error

  return ret;

}

int sys_gettime(){ 
  return zeos_ticks;
}

