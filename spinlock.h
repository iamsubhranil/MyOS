#pragma once

#include "myos.h"

struct SpinLock {
	volatile u32 lk;

	SpinLock() {
		lk = 0;
	}

	void lock() {
		while(!__sync_bool_compare_and_swap(&lk, 0, 1)) __sync_synchronize();
	}

	void unlock() {
		__sync_synchronize();
		lk = 0; // aligned writes are atomic
	}

	bool isLocked() {
		return lk != 0; // aligned cmps are atomic
	}
};
