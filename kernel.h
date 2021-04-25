#pragma once

#include "myos.h"

struct Kernel {
	struct Memory {
		static constexpr u32 Size = 0x1000000; // 16MiB for now
		static u32           placementAddress;
		static void *        alloc(u32 size);
		static void *alloc(u32 size, u32 *phys); // returns physical address
		// aligned alloc
		static void *alloc_a(u32 size);
		static void *alloc_a(u32 size, u32 *phys); // returns physical address
	};
};
