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
u64            Scheduler::TscTicksPerMs           = 0;
u64            Scheduler::TscTicksPerTimeSlice    = 0;

void Scheduler::prepare(Task *t, void *future_addr, void *future_set,
                        u32 numargs) {
	// PROMPT_INIT("Scheduler::prepare", Orange);
	// allocate a new stack
	t->stackptr = Memory::kalloc_a(Task::DefaultStackSize);
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

	t->pageDirectory = Paging::Directory::CurrentDirectory->clone();
	// PROMPT("here");
	t->heap.init(Task::DefaultHeapStart, Task::DefaultHeapSize,
	             t->pageDirectory);
}

void Scheduler::appendTask(Task *t) {
	bool suspended = false;
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
		case Task::New:
			suspend();
			suspended = true;
			// fall through
		case Task::Sleeping: // this is called from scheduler_scheduleNext, and
		                     // we don't really wanna resume inside of that
			t->state         = Task::State::Scheduled;
			t->next          = ReadyQueue->next;
			ReadyQueue->next = t;
			t->prev          = (Task *)ReadyQueue;
			t->next->prev    = t;
			ReadyQueue       = t;
			if(suspended)
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
		CurrentTask->state = Task::Waiting;
	if(unlock) {
		// unlock the lock
		lock.unlock();
	}
	resume_and_yield();
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

void Scheduler::sleep(u64 ms) {
	suspend();
	u64 currentTime = Asm::rdtsc();
	CurrentTask->elapsedTime +=
	    (currentTime - CurrentTask->lastStartTime) / TscTicksPerMs;
	// lastStartTime will now act as waitEndTime
	CurrentTask->lastStartTime = currentTime + (ms * TscTicksPerMs);
	// add us to the waiting queue
	volatile Task **task = &Scheduler::WaitingQueue;
	while(*task && (*task)->lastStartTime <= CurrentTask->lastStartTime)
		task = (volatile Task **)&(*task)->nextInList;
	CurrentTask->nextInList = (Task *)(*task);
	*task                   = CurrentTask;
	// remove ourselves from the list
	CurrentTask->prev->next = CurrentTask->next;
	CurrentTask->next->prev = CurrentTask->prev;
	if(ReadyQueue == CurrentTask)
		ReadyQueue = CurrentTask->next;
	CurrentTask->state = Task::State::Sleeping;
	resume_and_yield();
}

extern "C" {
extern uptr scheduler_scheduleNext(Register *oldRegisters) {
	// directly called from assembly, call will return
	// back to assembly to end
	u64 currentTime = Asm::rdtsc();
	// check if we have any tasks to wake from sleep.
	// waiting queue is an ordered queue.
	volatile Task **task = &Scheduler::WaitingQueue;
	// if the time we were supposed to wake up is less
	// than or equal to current time, add us to the ready queue
	while(*task && (*task)->lastStartTime <= currentTime) {
		Scheduler::appendTask((Task *)*task);
		// remove us from this list
		volatile Task *t = *task;
		*task            = (*task)->nextInList;
		t->nextInList    = NULL;
	}

	Task *currentTask = (Task *)Scheduler::CurrentTask;

	// only set the state to scheduled if it was running previously,
	// otherwise it may be the case that its state is already
	// modified, and a reschedule is forced
	if(currentTask->state == Task::State::Ready) {
		// this was for a scheduled task switch, so check if we
		// have consumed our timeslice
		if(!currentTask->yielded && currentTime - currentTask->lastStartTime <
		                                Scheduler::TscTicksPerTimeSlice) {
			// we have not yet consumed our slice
			// so just return for now
			return oldRegisters->useless_esp;
		}
		currentTask->state = Task::State::Scheduled;
	}
	currentTask->yielded = false;
	// update our elapsed time
	currentTask->elapsedTime +=
	    (currentTime - currentTask->lastStartTime) / Scheduler::TscTicksPerMs;
	// does not overwrite esp and ss
	*(Register *)&currentTask->regs = *oldRegisters;

	Scheduler::CurrentTask = currentTask = currentTask->next;

	currentTask->lastStartTime = currentTime;
	currentTask->state         = Task::State::Ready;

	if(Paging::Directory::CurrentDirectory != currentTask->pageDirectory)
		Paging::switchPageDirectory(currentTask->pageDirectory);
	// pass our sp into eax, so it does not get lost
	return currentTask->regs.useless_esp;
	// let the caller handle the next
}
}

void Scheduler::cleanupTask() {
	// PROMPT_INIT("CleanupTask", Orange);
	while(true) {
		// acquire the semaphore to make sure we have
		// tasks to be cleaned
		CleanupSemaphore.acquire();
		// for now, just release the stack
		// PROMPT("Cleaning up");
		Memory::kfree(FinishedTasks->stackptr);
		// Memory::kfree(FinishedTasks->heap);
		Task *OldFinishedTask = (Task *)FinishedTasks;
		// u32   oldId           = OldFinishedTask->id;
		FinishedTasks = FinishedTasks->nextInList;
		Memory::kfree(OldFinishedTask);
		// Terminal::write("Cleaned up: Task#", oldId, "\n");
	}
}

void Scheduler::init() {
	Asm::cli();
	PROMPT_INIT("Scheduler", Orange);
	PROMPT("Creating kernel task..");
	SchedulerLock    = SpinLock();
	CleanupSemaphore = Semaphore(0);
	Task *t          = Memory::kcreate<Task>();
	CurrentTask = ReadyQueue = t;
	t->state                 = Task::State::Scheduled;
	t->prev                  = t;
	t->next                  = t;
	t->pageDirectory         = Paging::Directory::KernelDirectory;
	t->heap                  = *Memory::kernelHeap;
	Memory::kernelHeap       = &t->heap;
	PROMPT("Initializing timer..");
	Timer::init();
	PROMPT("Calibrating TSC..");
	// calibrate TSC
	TscTicksPerMs        = Timer::calibrateTSC();
	TscTicksPerTimeSlice = TscTicksPerMs * TimeSliceMs;
	t->lastStartTime     = Asm::rdtsc();
	PROMPT("Waiting for the kernel task to be scheduled..");
	Asm::sti();
	while(CurrentTask->state == Task::State::Scheduled)
		;
	PROMPT("Task schedule complete!");
	PROMPT("Starting the cleanup task..");
	// start the cleanup task
	submit(cleanupTask);
	PROMPT("Initialization complete!");
}
