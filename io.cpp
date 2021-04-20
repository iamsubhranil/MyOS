#include "io.h"

void IO::outb(u16 port, u8 val) {
	asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
	/* There's an outb %al, $imm8  encoding, for compile-time constant port
	 * numbers that fit in 8b.  (N constraint). Wider immediate constants would
	 * be truncated at assemble-time (e.g. "i" constraint). The  outb  %al, %dx
	 * encoding is the only option for all other cases. %1 expands to %dx
	 * because  port  is a u16.  %w1 could be used if we had the port
	 * number a wider C type */
}

u8 IO::inb(u16 port) {
	u8 ret;
	asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
	return ret;
}

u8 IO::getScanCode() {
	u8 c = 0;
	do {
		if(inb(0x60) != c) {
			c = inb(0x60);
			if(c > 0)
				return c;
		}
	} while(1);
}
