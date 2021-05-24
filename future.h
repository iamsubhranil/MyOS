#pragma once

template <typename T> struct Future {
	T    value;
	bool isAvailable;

	Future() : isAvailable(false) {
	}

	void set(T v) {
		value       = v;
		isAvailable = true;
	}

	T get() {
		while(!isAvailable)
			;
		return value;
	}
};

template <> struct Future<void> {
	bool isAvailable;

	Future() : isAvailable(false) {
	}

	void set() {
		isAvailable = true;
	}

	void get() {
		while(!isAvailable)
			;
	}
};
;
