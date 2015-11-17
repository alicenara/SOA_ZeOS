#include <libc.h>
#include <stats.h>

void escriureInt(int i){
	char a[1000];
	itoa(i,a);
	int len = strlen(a);
  write(1, a,len);
}

void escriureString(char *a){
	int len = strlen(a);
  write(1, a,len);
}

void escriureEstats(int pid){
  struct stats st;
  get_stats(pid,&st);

  escriureString(" \n ");
	escriureString("Mostra de resultats: PID ");
	escriureInt(pid);
  escriureString("\n+ Total ticks -> ");
  escriureInt((int) st.elapsed_total_ticks);
  escriureString("\n - User ticks -> ");
  escriureInt((int) st.user_ticks);
  escriureString("\n - System ticks -> ");
  escriureInt((int) st.system_ticks);
  escriureString("\n - Ready ticks -> ");
  escriureInt((int) st.ready_ticks);
  escriureString("\n - Blocked ticks -> ");
  escriureInt((int) st.blocked_ticks);
}

void workload(int i){
	char a[100];
	int r = 0;
	if (i == 0){
		fork();
		fork();
		read(0,&a,10);
		for(r=0;r<10000;r++){}
	}else if (i==1){
		fork();
		fork();
		for(r=0;r<10000;r++){}
	}else{
		fork();
		fork();
		for(r=0;r<10000;r++){read(0,&a,10);}
	}
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
 //write(1,"pony",4);
	/*int i = fork();
	char a[100];
	itoa(i,a);
	int len = strlen(a);
  write(1, a,len);*/
  set_sched_policy(0);

  workload(0);

  escriureEstats(0);
  escriureEstats(1);
  escriureEstats(5);
  escriureEstats(6);
  
  //escriureEstats(getpid() - 1);
	
  while (1);
}
