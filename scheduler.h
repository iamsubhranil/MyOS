#pragma once

#include "asm.h"
#include "future.h"
#include "memory.h"
#include "semaphore.h"
#include "task.h"

struct Scheduler {

	static volatile Task *ReadyQueue;
	static volatile Task *WaitingQueue;
	static volatile Task *CurrentTask;

	// this will schedule the task, and block till the task
	// is finished, and then return the result
	template <typename T, typename... F>
	static T execute(T (*run)(F... args), F... args) {
		return submit(run, args...)->get();
	}

	// this schedules the task, but returns immediately.
	// returns an object which will contain the result,
	// once it is available.
	template <typename T, typename... F>
	static Future<T> *submit(T (*run)(F... args), F... args) {
		Future<T> *result = Memory::create<Future<T>>();
		auto       a      = [&run, &result, &args...]() -> void {
            result->set(run(args...));
            // after we are done executing what was required,
            // we will remove ourselves from the ready queue
            Scheduler::unschedule();
		};
		Task *t   = Memory::create<Task>();
		t->runner = a;
		schedule(t);
		return result;
	}

	// adds a task to the ready queue
	static void schedule(Task *t);
	// removes the current task from ready queue,
	// and hopefully, sometime in the future,
	// releases its resources
	static void unschedule();

	// will instruct the scheduler to move to the next task
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

	// we're not doing SMP yet, so this should suffice
	static inline void lock() {
		Asm::cli();
	}

	static inline void unlock() {
		Asm::sti();
	}

	// removes current task from readyqueue
	// and adds it to the wait queue.
	// proceeds the next task.
	// when the task is put back on the
	// readyqueue, it returns back.
	static void wait();

	// periodic scheduler, the first one does the
	// actual bookkeeping, the second one just performs
	// the switch.
	static void scheduleNext(Register *oldRegisters);
	static void switchNext(Task *newTask);

	static void init();
};
