#pragma once

#include "memory.h"
#include "myos.h"
#include "string.h"

template <typename T> struct OrderedArray {

	typedef bool (*lessThan)(const T &a, const T &b);

	static bool StandardLessThan(const T &a, const T &b) {
		return a < b;
	}

	T *      values;
	siz      size;
	siz      capacity;
	lessThan predicate;

	OrderedArray() {
		values = NULL;
		size = capacity = 0;
		predicate       = StandardLessThan;
	}
	// allocates memory
	OrderedArray(siz cap, lessThan lessThanPredicate = StandardLessThan) {
		size      = 0;
		capacity  = cap;
		predicate = lessThanPredicate;
		values    = (T *)Memory::alloc(capacity * sizeof(T));
		memset(values, 0, capacity * sizeof(T));
	}
	// creates in addr
	OrderedArray(void *addr, siz cap,
	             lessThan lessThanPredicate = StandardLessThan) {
		size      = 0;
		capacity  = cap;
		predicate = lessThanPredicate;
		values    = (T *)addr;
		memset(values, 0, capacity * sizeof(T));
	}

	void destroy() {
		// kfree
	}
	void insert(T item) {
		siz i = 0;
		while(i < size && predicate(values[i], item)) i++;
		if(i == size)
			values[size++] = item;
		else {
			T tmp     = values[i];
			values[i] = item;
			while(i < size) {
				i++;
				T tmp2    = values[i];
				values[i] = tmp;
				tmp       = tmp2;
			}
			size++;
		}
	}
	T get(siz index) {
		return values[index];
	}
	void remove(siz index) {
		siz i = index;
		while(i < size) {
			values[i] = values[i + 1];
			i++;
		}
		size--;
	}
};
