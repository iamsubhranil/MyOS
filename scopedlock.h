#pragma once

#include "spinlock.h"

struct ScopedLock {
	SpinLock &s;

	ScopedLock(SpinLock &l) : s(l) {
		s.lock();
	}

	~ScopedLock() {
		s.unlock();
	}
};
