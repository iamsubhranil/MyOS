#pragma once

#include <sys/system.h>

template <typename T, int N> class StaticQueue {
	T      values[N];
	size_t start, current;

  public:
	StaticQueue() {
		start   = 0;
		current = 0;
	}

	bool has() const {
		return current != start;
	}

	size_t size() const {
		if(current >= start)
			return current - start;
		else
			return (N - start) + current;
	}

	T get() {
		size_t old = start;
		start      = (start + 1) % N;
		return values[old];
	}

	void put(T value) {
		if(size() == N)
			return;
		values[current] = value;
		current         = (current + 1) % N;
	}
};
