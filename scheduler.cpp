#include "scheduler.h"
#include "asm.h"
#include "irq.h"
#include "terminal.h"
#include "timer.h"

extern "C" {
uptr        eip_load();
extern void performTaskSwitch(uptr ebx, uptr edx, uptr esi, uptr ebp, uptr esp,
                              uptr physAddr, uptr eip);
extern void __switch_task();
}

volatile Task *Scheduler::ReadyQueue   = NULL;
volatile Task *Scheduler::WaitingQueue = NULL;
volatile Task *Scheduler::CurrentTask  = NULL;

void Scheduler::schedule(Task *t) {
	lock();
	t->pageDirectory = Paging::Directory::CurrentDirectory->clone();
	t->next          = ReadyQueue->next;
	ReadyQueue->next = t;
	ReadyQueue       = t;
	// allocate a new stack
	Paging::switchPageDirectory(t->pageDirectory);
	uptr *newStack = (uptr *)Memory::alloc_a(Task::DefaultStackSize) +
	                 Task::DefaultStackSize / sizeof(uptr) - 1;
	*newStack-- = 0x0; // ebp will be pushed here
	// stack model for the interrupt handler
	// flags
	*newStack-- = 0x0;
	// cs
	*newStack-- = 0x0;
	// ip to jump
	*newStack-- = t->regs.eip = (uptr)&Task::begin;
	// error code and irq number
	*newStack-- = 0x0;
	*newStack-- = 0x20;
	// fs gs es ds
	*newStack-- = 0x10;
	*newStack-- = 0x10;
	*newStack-- = 0x10;
	*newStack   = 0x10;
	// set this as the new stack
	t->regs.esp = t->regs.useless_esp = (uptr)newStack;
	newStack--;
	// push 8 gpr's
	*newStack-- = 0xA;   // eax
	*newStack-- = 0xC;   // ecx
	*newStack-- = 0xD;   // edx
	*newStack-- = 0xB;   // ebx
	*newStack-- = 0xEAC; // esp
	*newStack-- = 0xEB;  // ebp

	*newStack-- = t->regs.esi = (uptr)t; // esi contains our argument

	*newStack-- = 0xED; // edi
	// now turn back to the old directory
	Paging::switchPageDirectory(CurrentTask->pageDirectory);
	unlock();
}

void Scheduler::unschedule() {
	lock();
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
	// yield from current task
	yield();
}

void Scheduler::yield(Task *to) {
	lock();
	// search for the parent of the to task
	Task *parent;
	for(parent = to; parent->next != to; parent = parent->next)
		;
	parent->next      = to->next;
	to->next          = CurrentTask->next;
	CurrentTask->next = to;
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

void Scheduler::scheduleNext(Register *oldRegisters) {
	scheduler_scheduleNext(oldRegisters);
}

void Scheduler::switchNext(Task *t) {
	asm volatile("mov %0, %%eax\n"
	             "jmp %1"
	             :
	             : "r"(t->regs.useless_esp), "m"(__switch_task)
	             : "eax");
}

void Scheduler::init() {
	Asm::cli();
	PROMPT_INIT("Scheduler", Brown);
	PROMPT("Creating kernel task..");
	Task *t     = Memory::create<Task>();
	CurrentTask = ReadyQueue = t;
	t->next                  = t;
	t->pageDirectory         = Paging::Directory::KernelDirectory;
	PROMPT("Installing scheduler handler..");
	IRQ::installHandler(0, scheduleNext);
	// we don't dump the registers here,
	// they will be loaded by the irq once
	// installed
	PROMPT("Waiting for the task to be scheduled..");
	Timer::init();
	asm volatile("sti");
	while(CurrentTask->state == Task::State::Scheduled)
		;
	PROMPT("Initialization complete!");
}
