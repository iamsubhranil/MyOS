#ifndef IO_HEADER
#define IO_HEADER

#include "myos.h"

struct IO {
	static u8 getScanCode();

	static u8 inb(u16 port);

	static void outb(u16 port, u8 val);
};

#endif
