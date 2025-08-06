#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

#define RT_RATIO 100
#define RR_RATIO 5

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;


extern struct {
	struct spinlock lock;
	struct proc proc[NPROC];
} ptable;


	void
tvinit(void)
{
	int i;

	for(i = 0; i < 256; i++)
		SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
	SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

	initlock(&tickslock, "time");
}

	void
idtinit(void)
{
	lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
	void
trap(struct trapframe *tf)
{
	if(tf->trapno == T_SYSCALL){
		if(myproc()->killed)
			exit();
		myproc()->tf = tf;
		syscall();
		if(myproc()->killed)
			exit();
		return;
	}

	switch(tf->trapno){
		case T_IRQ0 + IRQ_TIMER:
			if(cpuid() == 0){
				acquire(&tickslock);
				ticks++;
				wakeup(&ticks);
				release(&tickslock);
			}


			static int check = RR; 
			if(myproc() && myproc()->rt_rr == RT && myproc()->state == RUNNING && myproc()->my_rr_rt == RT)
			{
				myproc()->aging_time++;
				if(myproc()->aging_time == RT_RATIO)
				{
					cprintf("P%d aging_time=%d down\n", myproc()->pid, myproc()->aging_time);
					myproc()->rt_rr = RR;
					myproc()->aging_time = 0;
					check = RT;		 //강등상태
				}
			}
			if(myproc() && myproc()->state == RUNNING && check == RT)
			{
				myproc()->aging_time++;
				if(myproc()->aging_time == RR_RATIO)
				{
					cprintf("P%d aging_time=%d up\n", myproc()->pid, myproc()->aging_time);
					struct proc *p;
					acquire(&ptable.lock);
					for(p = ptable.proc ; p < &ptable.proc[NPROC] ; p++)
					{
						if(p->my_rr_rt == RT)
						{
							p->rt_rr = RT;
							break;
						}
					}
					release(&ptable.lock);
					myproc()->aging_time = 0;
					check = RR;
				}
			}



			lapiceoi();
			break;

		case T_IRQ0 + IRQ_IDE:
			ideintr();
			lapiceoi();
			break;
		case T_IRQ0 + IRQ_IDE+1:
			// Bochs generates spurious IDE1 interrupts.
			break;
		case T_IRQ0 + IRQ_KBD:
			kbdintr();
			lapiceoi();
			break;
		case T_IRQ0 + IRQ_COM1:
			uartintr();
			lapiceoi();
			break;
		case T_IRQ0 + 7:
		case T_IRQ0 + IRQ_SPURIOUS:
			cprintf("cpu%d: spurious interrupt at %x:%x\n",
					cpuid(), tf->cs, tf->eip);
			lapiceoi();
			break;

			//PAGEBREAK: 13
		default:
			if(myproc() == 0 || (tf->cs&3) == 0){
				// In kernel, it must be our mistake.
				cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
						tf->trapno, cpuid(), tf->eip, rcr2());
				panic("trap");
			}
			// In user space, assume process misbehaved.
			cprintf("pid %d %s: trap %d err %d on cpu %d "
					"eip 0x%x addr 0x%x--kill proc\n",
					myproc()->pid, myproc()->name, tf->trapno,
					tf->err, cpuid(), tf->eip, rcr2());
			myproc()->killed = 1;
	}

	// Force process exit if it has been killed and is in user space.
	// (If it is still executing in the kernel, let it keep running
	// until it gets to the regular system call return.)
	if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
		exit();

	// Force process to give up CPU on clock tick.
	// If interrupts were on while locks held, would need to check nlock.
	if(myproc() && myproc()->state == RUNNING &&
			tf->trapno == T_IRQ0+IRQ_TIMER)
		yield();

	// Check if the process has been killed since we yielded
	if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
		exit();
}
