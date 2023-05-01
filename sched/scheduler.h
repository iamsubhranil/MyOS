#pragma once

#include <arch/x86/asm.h>
#include <mem/memory.h>
#include <sched/future.h>
#include <sched/scopedlock.h>
#include <sched/semaphore.h>
#include <sched/spinlock.h>
#include <sched/task.h>

struct Scheduler {

	static volatile Task *ReadyQueue;
	static volatile Task *CurrentTask;
	static volatile Task *FinishedTasks;
	static SpinLock       SchedulerLock;
	static Semaphore      CleanupSemaphore;

	// amount of time each task should run before it is switched
	static const u64 TimeSliceMs = 150;
	// priority queue based on lastStartTime, which acts as
	// time of resume
	static volatile Task *WaitingQueue;

	// increment in tsc per milisecond, calibrated once for now
	static u64 TscTicksPerMs;
	static u64 TscTicksPerTimeSlice;

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
		// result has to be accessible from both the tasks
		Future<T> *result = Memory::kcreate<Future<T>>();
		Task      *t      = Memory::kcreate<Task>();
		t->runner         = (void *)run;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpmf-conversions"
		prepare(t, (void *)result, (void *)&Future<T>::set, sizeof...(args));
#pragma GCC diagnostic pop
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
		CurrentTask->yielded = true;
		// enable interrupts if it wasn't already
		// and then call the scheduler interrupt
		asm volatile("sti\n"
		             "int $0x20\n");
	}

	static void resume_and_yield() {
		RecursiveSuspendCounter--;
		CurrentTask->yielded = true;
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

	static void sleep(u64 ms);

	static void init();
};
