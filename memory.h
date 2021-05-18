#pragma once

#include "myos.h"

struct Heap;
struct Memory {

	static siz Size; // set by Paging::Frame::init by reading the multiboot map
	static Heap *kernelHeap;
	static uptr  placementAddress;
	// to get the physical address, use Paging::getPhysicalAddress
	static void *alloc(siz size);
	// aligned alloc
	static void *alloc_a(siz size);

	static void free(void *addr); // only works when heap is active

	template <typename T, typename... F> static T *create(F... args) {
		T *val = (T *)alloc(sizeof(T));
		(*val) = T(args...);
		return val;
	}
};
