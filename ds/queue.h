#pragma once

#include <mem/memory.h>
#include <sched/semaphore.h>

template <typename T> class Queue {
	T        *values;
	T         defaultNull;
	size_t    capacity, current;
	Semaphore sem;

  public:
	Queue() {
		values   = (T *)Memory::alloc(sizeof(T));
		capacity = 1;
		current  = 0;
		sem      = Semaphore(0);
	}

	T get() {
		sem.acquire();
		return values[--current];
	}

	void put(T value) {
		values[current++] = value;
		sem.release();
	}
};
