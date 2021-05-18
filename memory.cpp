#include "memory.h"
#include "heap.h"
#include "kernel_layout.h"
#include "paging.h"

extern u32 __ld_kernel_end; // defined in linker script
uptr       Memory::placementAddress = (uptr)&__ld_kernel_end;
Heap *     Memory::kernelHeap       = NULL;
siz        Memory::Size             = 0;

void *Memory::alloc(siz size) {
	if(kernelHeap) {
		return kernelHeap->alloc(size, false);
	}
	return (void *)((uptr)(placementAddress += size) - size);
}

void *Memory::alloc_a(siz size) {
	if(kernelHeap) {
		return kernelHeap->alloc(size, true);
	}
	Paging::alignIfNeeded(placementAddress);
	return alloc(size);
}

void Memory::free(void *addr) {
	if(kernelHeap) {
		kernelHeap->free(addr);
	}
}
