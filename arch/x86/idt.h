#pragma once

#include <sys/myos.h>

struct IDT {
	struct Entry {
		u16 base_lo;
		u16 sel;     /* Our kernel segment goes here! */
		u8  always0; /* This will ALWAYS be set to 0! */
		u8  flags;   /* Set using the above table! */
		u16 base_hi;
	} __attribute__((packed));

	struct Pointer {
		u16  limit;
		uptr base;
	} __attribute__((packed));

	static Entry       entries[256];
	static Pointer     __idtptr;
	static const char *exceptionMessages[32];

	static void setGate(u8 num, uptr base, u16 sel, u8 flags);
	static void init();
};
