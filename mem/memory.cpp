#include <arch/x86/kernel_layout.h>
#include <mem/heap.h>
#include <mem/memory.h>
#include <mem/paging.h>
#include <sched/scheduler.h>
#include <sys/string.h>

extern u32 __ld_kernel_end; // defined in linker script
uptr       Memory::placementAddress = (uptr)&__ld_kernel_end;
Heap      *Memory::kernelHeap       = NULL;
siz        Memory::Size             = 0;

// we need to do the pointless & and * because CurrentTask is
// volatile, but the heap functions are not. that is fine,
// because this function will only run in the context
// of one task.
void *Memory::alloc(siz size) {
	return ((Heap *)(&Scheduler::CurrentTask->heap))->alloc(size);
}

void *Memory::kalloc(siz size) {
	return kernelHeap->alloc(size);
}

void *Memory::kalloc_noheap(siz size) {
	return (void *)((uptr)(placementAddress += size) - size);
}

void *Memory::alloc_a(siz size) {
	return ((Heap *)(&Scheduler::CurrentTask->heap))->alloc_a(size);
}

void *Memory::kalloc_a(siz size) {
	return kernelHeap->alloc_a(size);
}

void *Memory::kalloc_anoheap(siz size) {
	Paging::alignIfNeeded(placementAddress);
	return kalloc_noheap(size);
}

void Memory::kfree(void *addr) {
	kernelHeap->free(addr);
}

void Memory::free(void *addr) {
	((Heap *)(&Scheduler::CurrentTask->heap))->free(addr);
}

extern "C" {
void *malloc(size_t size) {
	return Memory::alloc(size);
}

void free(void *addr) {
	Memory::free(addr);
}
}
