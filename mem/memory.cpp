#include <arch/x86/kernel_layout.h>
#include <mem/heap.h>
#include <mem/memory.h>
#include <mem/paging.h>
#include <sys/string.h>

extern u32 __ld_kernel_end; // defined in linker script
uptr       Memory::placementAddress = (uptr)&__ld_kernel_end;
Heap *     Memory::kernelHeap       = NULL;
siz        Memory::Size             = 0;

void *Memory::alloc(siz size) {
	if(kernelHeap) {
		return kernelHeap->alloc(size);
	}
	return (void *)((uptr)(placementAddress += size) - size);
}

void *Memory::alloc_a(siz size) {
	if(kernelHeap) {
		return kernelHeap->alloc_a(size);
	}
	Paging::alignIfNeeded(placementAddress);
	return alloc(size);
}

void Memory::free(void *addr) {
	if(kernelHeap) {
		kernelHeap->free(addr);
	}
}

extern "C" {
void *malloc(size_t size) {
	return Memory::alloc(size);
}

void free(void *addr) {
	Memory::free(addr);
}
}
