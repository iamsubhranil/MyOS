#pragma once

#include <sched/spinlock.h>
#include <sched/task.h>

struct Semaphore {
	i32 value;
	// list of tasks which are waiting on this semaphore
	Task *   taskList;
	SpinLock lock;

	Semaphore(u32 initial) {
		value    = initial;
		taskList = NULL;
		lock     = SpinLock();
	}
	Semaphore() : Semaphore(1) {
	}
	void acquire();
	// if lock is false, the semaphore won't be locked
	// before release. only pass false if the caller can
	// ensure noninterruptable context.
	void release(u32 times = 1, bool lock = true);
};
