#pragma once

#include "asm.h"
#include "future.h"
#include "memory.h"
#include "scopedlock.h"
#include "semaphore.h"
#include "spinlock.h"
#include "task.h"

struct Scheduler {

	static volatile Task *ReadyQueue;
	static volatile Task *WaitingQueue;
	static volatile Task *CurrentTask;
	static volatile Task *FinishedTasks;
	static SpinLock       SchedulerLock;
	static Semaphore      CleanupSemaphore;

	// this will schedule the task, and block till the task
	// is finished, and then return the result
	template <typename T, typename... F>
	static T execute(T (*run)(F... args), F... args) {
		return submit(run, args...)->get();
	}

	static void populateStack(uptr *&stk) {
		(void)stk;
	}
	template <typename F, typename... T>
	static void populateStack(uptr *&stk, F arg, T... args) {
		populateStack(stk, args...);
		*stk = (uptr)arg;
		stk--;
	}

	// this schedules the task, but returns immediately.
	// returns an object which will contain the result,
	// once it is available.
	template <typename T, typename... F>
	static Future<T> *submit(T (*run)(F... args), F... args) {
		Future<T> *result = Memory::create<Future<T>>();
		Task *     t      = Memory::create<Task>();
		t->runner         = (void *)run;
		prepare(t, (void *)result, (void *)&Future<T>::set, sizeof...(args));
		uptr *stk = (uptr *)t->regs.useless_esp;
		// skip the registers
		stk -= 8;
		// switch page directory
		Paging::switchPageDirectory(t->pageDirectory);
		populateStack(stk, args...);
		Paging::switchPageDirectory(CurrentTask->pageDirectory);
		appendTask(t);
		return result;
	}

	// counts the recursion in suspend/resume
	static u32 RecursiveSuspendCounter;
	// temporarily suspends the task switch
	static void suspend() {
		Asm::cli();
		RecursiveSuspendCounter++;
	}
	// resumes task switch if it was suspended
	static void resume() {
		RecursiveSuspendCounter--;
		if(RecursiveSuspendCounter == 0)
			Asm::sti();
	}

	// prepares a new task to be added the ready queue
	// future_addr contains the address of the future which
	// will store the return value.
	// future_set contains the address of the Future::set
	// function, which will be called to set the return
	// value, and wake any waiting tasks.
	// number of arguments to the task, used to prepare
	// the stack for a new task initialization.
	// if immediate is false, schedule does not enable
	// interrupts before returning, and leaves that task
	// upto the caller.
	// it does not acquire a lock. that is upto the caller.
	static void prepare(Task *t, void *future_addr, void *future_set,
	                    u32 numargs);
	// appends a new task in the ready queue.
	// if the task state of the task is Unscheduled,
	// it just marks it as scheduled and returns
	static void appendTask(Task *t);
	// removes the current task from ready queue,
	// and hopefully, sometime in the future,
	// releases its resources.
	// if finished is true, this will mark the task
	// as Finished, and its resources will be
	// released sometime in the future.
	static void unschedule(bool finished = false);
	// releases the lock before calling yield
	static void unschedule(SpinLock &lock, bool finished = false);
	// core implementation of unscheduler
	static void unschedule_(bool      finished = false,
	                        SpinLock &lock     = Scheduler::SchedulerLock,
	                        bool      unlock   = false);
	static void finish();

	// will instruct the scheduler to move to the next task,
	// saving the state of the current task.
	static void yield() {
		// enable interrupts if it wasn't already
		// and then call the scheduler interrupt
		asm volatile("sti\n"
		             "int $0x20\n");
	}
	// will instruct the scheduler to move to the
	// specified task. first, pointers will be adjusted
	// to make that task is the next task of CurrentTask,
	// then an yield will be called
	static void yield(Task *t);

	static volatile Task *getCurrentTask() {
		return CurrentTask;
	}

	// cleans up resources used by finished tasks
	static void cleanupTask();

	static void init();
};
