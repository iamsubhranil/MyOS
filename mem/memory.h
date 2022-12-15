#pragma once

#include <sys/myos.h>

struct Heap;
struct Memory {

	static siz Size; // set by Paging::Frame::init by reading the multiboot map
	static Heap *kernelHeap;
	static uptr  placementAddress;

	// to get the physical address, use Paging::getPhysicalAddress
	static void *alloc(siz size);  // allocates from task heap
	static void *kalloc(siz size); // allocates from the kernel heap
	static void *kalloc_noheap(
	    siz size); // for allocating memory until the heap is active

	// aligned alloc
	static void *alloc_a(siz size);  // allocates from task heap
	static void *kalloc_a(siz size); // allocates from the kernel heap
	static void *
	    kalloc_anoheap(siz size); // allocating until the heap is active

	static void free(void *addr);  // only works when heap is active
	static void kfree(void *addr); // frees kalloc'd memory

	template <typename T, typename... F> static T *create(F... args) {
		T *val = (T *)alloc(sizeof(T));
		(*val) = T(args...);
		return val;
	}

	template <typename T, typename... F> static T *kcreate(F... args) {
		T *val = (T *)kalloc(sizeof(T));
		(*val) = T(args...);
		return val;
	}
};
