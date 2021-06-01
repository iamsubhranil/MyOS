#pragma once

#include <sched/spinlock.h>

struct ScopedLock {
	SpinLock &s;

	ScopedLock(SpinLock &l) : s(l) {
		s.lock();
	}

	~ScopedLock() {
		s.unlock();
	}
};
