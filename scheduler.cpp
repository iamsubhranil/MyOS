#include "scheduler.h"
#include "asm.h"
#include "irq.h"
#include "terminal.h"
#include "timer.h"

volatile Task *Scheduler::ReadyQueue    = NULL;
volatile Task *Scheduler::WaitingQueue  = NULL;
volatile Task *Scheduler::CurrentTask   = NULL;
SpinLock       Scheduler::SchedulerLock = SpinLock();

void Scheduler::prepare(Task *t, void *future_addr, void *future_set,
                        u32 numargs) {
	t->pageDirectory = Paging::Directory::CurrentDirectory->clone();
	// allocate a new stack
	Paging::switchPageDirectory(t->pageDirectory);
	uptr *newStack = (uptr *)Memory::alloc_a(Task::DefaultStackSize) +
	                 Task::DefaultStackSize / sizeof(uptr) - 1;
	// stack for task finish
	*newStack-- = 0x0; // empty slot to store eax
	*newStack-- = (uptr)future_addr;
	*newStack-- =
	    (uptr)&Scheduler::unschedule; // return address from Future<X>::set
	*newStack-- = (uptr)future_set;   // address of Future<X>::set
	// stack for task begin
	*newStack-- = numargs;
	// stack model for the interrupt handler
	// flags
	*newStack-- = 0x200; // enable interrupts
	// cs
	*newStack-- = 0x0;
	// ip to jump
	*newStack-- = t->regs.eip = (uptr)t->runner;
	// error code
	*newStack-- = 0x0;
	// to make the assembly scheduler notify that this is a new task
	// and it needs some special stack arrangements, we change the
	// interrupt number to TASK (hex). the scheduler will compare
	// the interrupt number, and jump to a special routine to
	// manage the arguments on the stack if it matches with this.
	*newStack-- = 0x5441534b;
	// fs gs es ds
	*newStack-- = 0x10;
	*newStack-- = 0x10;
	*newStack-- = 0x10;
	*newStack   = 0x10;
	// set this as the new stack
	t->regs.esp = t->regs.useless_esp = (uptr)newStack;
	newStack--;
	// push 8 gpr's
	*newStack-- = 0x0; // eax
	*newStack-- = 0x0; // ecx
	*newStack-- = 0x0; // edx
	*newStack-- = 0x0; // ebx
	*newStack-- = 0x0; // esp
	*newStack-- = 0x0; // ebp
	*newStack-- = 0x0; // esi
	*newStack-- = 0x0; // edi
	// now turn back to the old directory
	Paging::switchPageDirectory(CurrentTask->pageDirectory);
}

void Scheduler::appendTask(Task *t) {
	ScopedLock sl(SchedulerLock);
	suspend();
	t->next          = ReadyQueue->next;
	ReadyQueue->next = t;
	ReadyQueue       = t;
	resume();
}

void Scheduler::unschedule() {
	SchedulerLock.lock();
	volatile Task *parent;
	for(parent = CurrentTask; parent->next != CurrentTask;
	    parent = parent->next)
		;
	if(parent == CurrentTask) {
		Terminal::err("Cannot unschedule only task!");
		for(;;)
			;
	}
	// MEMORY LEAK
	parent->next = CurrentTask->next;
	if(ReadyQueue == CurrentTask) {
		ReadyQueue = CurrentTask->next;
	}
	SchedulerLock.unlock();
	// immediately jump to the next task
	yield();
}

void Scheduler::yield(Task *to) {
	SchedulerLock.lock();
	// search for the parent of the to task
	Task *parent;
	for(parent = to; parent->next != to; parent = parent->next)
		;
	parent->next      = to->next;
	to->next          = CurrentTask->next;
	CurrentTask->next = to;
	SchedulerLock.unlock();
	yield();
}

extern "C" {
extern uptr scheduler_scheduleNext(Register *oldRegisters) {
	// directly called from assembly, call will return
	// back to assembly to end
	// Terminal::write("scheduling..\nold: ", (u32)Scheduler::CurrentTask->id,
	//                "\n");
	// oldRegisters->dump();
	Scheduler::CurrentTask->state = Task::State::Waiting;
	// does not overwrite esp and ss
	*(Register *)&Scheduler::CurrentTask->regs = *oldRegisters;
	// memcpy((Register *)&Scheduler::CurrentTask->regs, oldRegisters,
	//       sizeof(Register));
	Scheduler::CurrentTask        = Scheduler::CurrentTask->next;
	Scheduler::CurrentTask->state = Task::State::Ready;
	// does not overwrite esp and ss
	// *oldRegisters = *(Register *)&Scheduler::CurrentTask->regs;
	// memcpy(oldRegisters, (void *)&Scheduler::CurrentTask->regs,
	//       sizeof(Register));
	if(Paging::Directory::CurrentDirectory !=
	   Scheduler::CurrentTask->pageDirectory)
		Paging::switchPageDirectory(Scheduler::CurrentTask->pageDirectory);
	// Terminal::write("\nnew: ", (u32)Scheduler::CurrentTask->id, ": ");
	// oldRegisters->dump();
	// Terminal::write("\n");
	// pass our sp into eax, so it does not get lost
	return Scheduler::CurrentTask->regs.useless_esp;
	// let the caller handle the next
}
}

void Scheduler::init() {
	Asm::cli();
	PROMPT_INIT("Scheduler", Brown);
	PROMPT("Creating kernel task..");
	SchedulerLock = SpinLock();
	Task *t       = Memory::create<Task>();
	CurrentTask = ReadyQueue = t;
	t->next                  = t;
	t->pageDirectory         = Paging::Directory::KernelDirectory;
	// we don't dump the registers here,
	// they will be loaded by the irq once
	// installed
	PROMPT("Waiting for the task to be scheduled..");
	Timer::init();
	Asm::sti();
	while(CurrentTask->state == Task::State::Scheduled)
		;
	PROMPT("Initialization complete!");
}
