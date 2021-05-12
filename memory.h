#pragma once

#include "myos.h"

struct Heap;
struct Memory {

	static constexpr siz Size = 0x1000000; // 16MiB for now
	static Heap *        kernelHeap;
	static uptr          placementAddress;
	static void *        alloc(siz size);
	static void *alloc(siz size, uptr &phys); // returns physical address
	// aligned alloc
	static void *alloc_a(siz size);
	static void *alloc_a(siz size, uptr &phys); // returns physical address

	static void free(void *addr); // only works when heap is active
};
