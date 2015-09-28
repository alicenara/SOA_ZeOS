#include <libc.h>

char buff[24];

int pid;

int add(int par1,int par2){
  int ret;
  __asm__("add %%ebx,%%eax\n\t"
      /*"mov %esp, %ebp\n\t"
      "pop %ebp\n\t"
      "ret\n\t"*/
     : "=a" (ret)
     : "a" (par1), "b" (par2)
     );
  return ret;
}

long inner(long n)
{
  int i;
  long suma;
  suma = 0;
  for (i=0; i<n; i++) suma = suma + i;
  return suma;
}

long outer(long n)
{
  int i;
  long acum;
  acum = 0;
  for (i=0; i<n; i++) acum = acum + inner(i);
  return acum;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

    
  /*long count, acum;
  count = 75;
  acum = 0;
  acum = add(1,2);
  acum = outer(count);*/
  write(1,"pony",4);
  write(1,"2pony2",6);
  int i = gettime();
  while (1);
  return 0;
}