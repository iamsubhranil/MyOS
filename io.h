#ifndef IO_HEADER
#define IO_HEADER

#include "asm.h"
#include "myos.h"

struct IO {
	static u8 getScanCode();

	static inline u8 inb(u16 port) {
		return Asm::inb(port);
	}

	static inline void outb(u16 port, u8 val) {
		Asm::outb(port, val);
	}
};

#endif
