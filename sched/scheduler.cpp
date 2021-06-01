#include <arch/x86/asm.h>
#include <drivers/terminal.h>
#include <drivers/timer.h>
#include <sched/scheduler.h>

volatile Task *Scheduler::ReadyQueue              = NULL;
volatile Task *Scheduler::WaitingQueue            = NULL;
volatile Task *Scheduler::CurrentTask             = NULL;
volatile Task *Scheduler::FinishedTasks           = NULL;
SpinLock       Scheduler::SchedulerLock           = SpinLock();
Semaphore      Scheduler::CleanupSemaphore        = Semaphore();
u32            Scheduler::RecursiveSuspendCounter = 0;

void Scheduler::prepare(Task *t, void *future_addr, void *future_set,
                        u32 numargs) {
	t->pageDirectory = Paging::Directory::CurrentDirectory->clone();
	// allocate a new stack
	Paging::switchPageDirectory(t->pageDirectory);
	t->stackptr = Memory::alloc_a(Task::DefaultStackSize);
	uptr *newStack =
	    (uptr *)(t->stackptr) + Task::DefaultStackSize / sizeof(uptr) - 1;
	// stack for task finish
	*newStack-- = 0x0; // empty slot to store eax
	*newStack-- = (uptr)future_addr;
	*newStack-- =
	    (uptr)&Scheduler::finish;   // return address from Future<X>::set
	*newStack-- = (uptr)future_set; // address of Future<X>::set
	// stack for task begin
	*newStack-- = numargs;
	// stack model for the interrupt handler
	// flags
	*newStack-- = 0x200; // enable interrupts
	// cs
	*newStack-- = 0x08;
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
	switch(t->state) {
		case Task::Finished:
			Terminal::err("Trying to schedule finished task!");
			break;
		case Task::Ready:
			Terminal::warn("Trying to schedule a ready task!");
			break;
		case Task::Scheduled:
			Terminal::warn("Task ", t->id, " is already scheduled!");
			break;
		case Task::Waiting:
		case Task::Unscheduled:
		case Task::New:
			suspend();
			t->state         = Task::State::Scheduled;
			t->next          = ReadyQueue->next;
			ReadyQueue->next = t;
			t->prev          = (Task *)ReadyQueue;
			t->next->prev    = t;
			ReadyQueue       = t;
			resume();
			break;
	};
}

void Scheduler::unschedule_(bool isFinished, SpinLock &lock, bool unlock) {
	suspend();
	// remove it from the list
	CurrentTask->prev->next = CurrentTask->next;
	CurrentTask->next->prev = CurrentTask->prev;
	if(ReadyQueue == CurrentTask)
		ReadyQueue = CurrentTask->next;
	if(isFinished) {
		// mark current task as finished
		CurrentTask->state = Task::Finished;
		// add it to the finished queue
		Task **finishedTasksLast = (Task **)&FinishedTasks;
		while(*finishedTasksLast) {
			finishedTasksLast = &(*finishedTasksLast)->nextInList;
		}
		*finishedTasksLast      = (Task *)CurrentTask;
		CurrentTask->nextInList = NULL;
		// wake up cleanup task if it was sleeping
		CleanupSemaphore.release(1, false);
	} else
		// mark the current task as unscheduled
		CurrentTask->state = Task::Unscheduled;
	if(unlock) {
		// unlock the lock
		lock.unlock();
	}
	resume();
	// yield
	yield();
}

void Scheduler::unschedule(bool isFinished) {
	unschedule_(isFinished);
}

void Scheduler::unschedule(SpinLock &lock, bool isFinished) {
	unschedule_(isFinished, lock, true);
}

void Scheduler::finish() {
	unschedule(true);
}

extern "C" {
extern uptr scheduler_scheduleNext(Register *oldRegisters) {
	// directly called from assembly, call will return
	// back to assembly to end

	// only set the state to scheduled if it was running previously,
	// otherwise it may be the case that its state is already
	// modified, and a reschedule is forced
	if(Scheduler::CurrentTask->state == Task::State::Ready)
		Scheduler::CurrentTask->state = Task::State::Scheduled;
	// does not overwrite esp and ss
	*(Register *)&Scheduler::CurrentTask->regs = *oldRegisters;

	Scheduler::CurrentTask = Scheduler::CurrentTask->next;

	Scheduler::CurrentTask->state = Task::State::Ready;

	if(Paging::Directory::CurrentDirectory !=
	   Scheduler::CurrentTask->pageDirectory)
		Paging::switchPageDirectory(Scheduler::CurrentTask->pageDirectory);
	// pass our sp into eax, so it does not get lost
	return Scheduler::CurrentTask->regs.useless_esp;
	// let the caller handle the next
}
}

void Scheduler::cleanupTask() {
	while(true) {
		// acquire the semaphore to make sure we have
		// tasks to be cleaned
		CleanupSemaphore.acquire();
		// for now, just release the stack
		Memory::free(FinishedTasks->stackptr);
		FinishedTasks = FinishedTasks->nextInList;
	}
}

void Scheduler::init() {
	Asm::cli();
	PROMPT_INIT("Scheduler", Brown);
	PROMPT("Creating kernel task..");
	SchedulerLock    = SpinLock();
	CleanupSemaphore = Semaphore(0);
	Task *t          = Memory::create<Task>();
	CurrentTask = ReadyQueue = t;
	t->state                 = Task::State::Scheduled;
	t->prev                  = t;
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
	// start the cleanup task
	submit(cleanupTask);
	PROMPT("Initialization complete!");
}
