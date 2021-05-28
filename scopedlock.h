#pragma once

#include "spinlock.h"

struct ScopedLock {
	SpinLock s;

	ScopedLock() noexcept {
		s = SpinLock();
		s.lock();
	}

	ScopedLock(SpinLock l) noexcept {
		s = l;
		s.lock();
	}

	~ScopedLock() noexcept {
		s.unlock();
	}
};
