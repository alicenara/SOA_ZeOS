/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

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

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
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
    
    while (size >= myMaxBuff){
      int comprovacio = copy_from_user(buffer, buff, myMaxBuff);
      if(comprovacio){
        ret = comprovacio; //error, buscar codi error;
        size = myMaxBuff;
      } else{
        int sizeEscrit = sys_write_console(mybuff, myMaxBuff);

        if(sizeEscrit < 0){
          ret = sizeEscrit; //error, buscar codi error;
          size = myMaxBuff;
        }else{
          numbytes += sizeEscrit;
          buffer += myMaxBuff;
          size -= myMaxBuff;
        }         
      }      
    }
    // volta final
    int comprovacio = copy_from_user(buffer, buff, myMaxBuff);
    if(comprovacio) ret = comprovacio; //error, buscar codi error;
    else{
      int sizeEscrit = sys_write_console(mybuff, myMaxBuff);

      if(sizeEscrit < 0) ret = sizeEscrit; //error, buscar codi error;
      else{
        numbytes += sizeEscrit;
        buffer += myMaxBuff;
        size -= myMaxBuff;
      }
    }

    if (numbytes != sizeOriginal) ret = -1; //buscar codi error
    else ret = numbytes;
  }else ret = -1; //buscar error

  return ret;

}

int sys_clock(){
  return zeos_ticks;

}

