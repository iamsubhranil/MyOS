#pragma once

#include <sys/myos.h>

struct GDT {

	/* Defines a GDT entry. We say packed, because it prevents the
	 *  compiler from doing things that it thinks is best: Prevent
	 *  compiler "optimization" by packing */
	struct Entry {
		u16 limit_low;
		u16 base_low;
		u8  base_middle;
		u8  access;
		u8  granularity;
		u8  base_high;
	} __attribute__((packed));

	/* Special pointer which includes the limit: The max bytes
	 *  taken up by the GDT, minus 1. Again, this NEEDS to be packed */
	struct Pointer {
		u16  limit;
		uptr base;
	} __attribute__((packed));

	static Entry entries[6];

	static void setGate(u32 num, u64 base, u64 limit, u8 access, u8 gran);

	static void encodeEntry(u8 *dest, GDT descriptor);
	static void init();
};
