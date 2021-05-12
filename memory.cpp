#include "memory.h"
#include "heap.h"
#include "kernel_layout.h"
#include "paging.h"

extern u32 __ld_kernel_end; // defined in linker script
uptr       Memory::placementAddress = (uptr)&__ld_kernel_end;
Heap *     Memory::kernelHeap       = NULL;

void *Memory::alloc(siz size) {
	if(kernelHeap) {
		return kernelHeap->alloc(size, false);
	}
	return (void *)((uptr)(placementAddress += size) - size);
}
void *Memory::alloc(siz size, uptr &phys) {
	if(kernelHeap) {
		void *        addr = kernelHeap->alloc(size, false);
		Paging::Page *page = Paging::getPage(
		    (uptr)addr, false, Paging::Directory::KernelDirectory);
		phys = page->frame * Paging::PageSize +
		       ((uptr)addr & (Paging::PageSize - 1));
		return addr;
	}
	phys = V2P(placementAddress);
	return alloc(size);
}

void *Memory::alloc_a(siz size) {
	if(kernelHeap) {
		return kernelHeap->alloc(size, true);
	}
	Paging::alignIfNeeded(placementAddress);
	return alloc(size);
}

void *Memory::alloc_a(siz size, uptr &phys) {
	if(kernelHeap) {
		void *        addr = kernelHeap->alloc(size, true);
		Paging::Page *page = Paging::getPage(
		    (uptr)addr, false, Paging::Directory::KernelDirectory);
		phys = page->frame * Paging::PageSize +
		       ((uptr)addr & (Paging::PageSize - 1));
		return addr;
	}
	Paging::alignIfNeeded(placementAddress);
	phys = V2P(placementAddress);
	return alloc(size);
}

void Memory::free(void *addr) {
	if(kernelHeap) {
		kernelHeap->free(addr);
	}
}
