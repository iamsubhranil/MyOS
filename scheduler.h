#pragma once

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
		Scheduler::yield(CurrentTask->next);
	}
	// will instruct the scheduler to move to the
	// specified task.
	// if a state is specified, current task will
	// resume from that state next time. otherwise,
	// the registers will be dumped inside 'yield',
	// and those will be used.
	// if finishIrq is true, we will send an EOI
	// signal to the current Irq device before the
	// switch.
	static void yield(Task *t, Register *currentRegisters = NULL,
	                  bool finishIrq = false);

	// will put the current task in the waiting queue,
	// and resume once the semaphore is available
	static void wait(Semaphore *s);

	static Task *getCurrentTask();

	// periodic scheduler, the first one does the
	// actual bookkeeping, the second one just performs
	// the switch.
	static void scheduleNext(Register *oldRegisters);
	static void switchNext(Task *newTask);

	static void init();
};
