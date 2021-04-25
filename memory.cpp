#include "kernel.h"
#include "paging.h"

extern u32 end; // defined in linker script
u32        Kernel::Memory::placementAddress = (u32)&end;

void *Kernel::Memory::alloc(u32 size) {
	return (void *)((placementAddress += size) - size);
}
void *Kernel::Memory::alloc(u32 size, u32 *phys) {
	*phys = placementAddress;
	return alloc(size);
}

void *Kernel::Memory::alloc_a(u32 size) {
	if(!Paging::isAligned(placementAddress)) {
		placementAddress &= ~(Paging::PageSize - 1);
		placementAddress += Paging::PageSize;
	}
	return alloc(size);
}

void *Kernel::Memory::alloc_a(u32 size, u32 *phys) {
	if(!Paging::isAligned(placementAddress)) {
		placementAddress &= ~(Paging::PageSize - 1);
		placementAddress += Paging::PageSize;
	}
	*phys = placementAddress;
	return alloc(size);
}
