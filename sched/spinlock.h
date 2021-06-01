#pragma once

#include <sys/myos.h>

struct Task;

struct SpinLock {
	volatile u8    lk;
	volatile Task *acquired;

	SpinLock() {
		lk = 0;
	}

	void lock() {
		while(!__sync_bool_compare_and_swap(&lk, 0, 1)) __sync_synchronize();
		assignAcquired();
	}

	void assignAcquired();

	void unlock() {
		__sync_synchronize();
		acquired = NULL;
		lk       = 0; // aligned writes are atomic
	}

	bool isLocked() {
		return lk != 0; // aligned cmps are atomic
	}
};
