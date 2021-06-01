#pragma once

#include <sched/scopedlock.h>
#include <sched/spinlock.h>
#include <sched/task.h>

struct FutureBase {
	bool     isAvailable;
	SpinLock lock;
	Task *   waitingTasks;

	FutureBase();

	void awakeAllNoLock();
	void waitTillAvailable();
};

template <typename T> struct Future : FutureBase {
	T value;

	Future() : FutureBase() {
	}

	void set(T v) {
		ScopedLock sl(lock);
		value = v;
		awakeAllNoLock();
	}

	T get() {
		waitTillAvailable();
		return value;
	}
};

template <> struct Future<void> : FutureBase {

	Future() : FutureBase() {
	}

	void set() {
		ScopedLock l(lock);
		awakeAllNoLock();
	}

	void get() {
		waitTillAvailable();
	}
};
