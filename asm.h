#pragma once

#include "myos.h"

struct Asm {
	static inline u32 bsf(u32 value) {
		u32 offset;
		asm("bsf %0, %1" : "=r"(offset) : "r"(value));
		return offset;
	}

	template <typename T> static inline void lidt(T ptr) {
		asm("lidt %0" : : "m"(ptr));
	}

	static inline void cli() {
		asm("cli");
	}

	static inline void sti() {
		asm("sti");
	};

	static inline u8 inb(u16 port) {
		u8 ret;
		asm("inb %1, %0" : "=a"(ret) : "Nd"(port));
		return ret;
	}

	static inline void outb(u16 port, u8 val) {
		asm("outb %0, %1" : : "a"(val), "Nd"(port));
		/* There's an outb %al, $imm8  encoding, for compile-time constant port
		 * numbers that fit in 8b.  (N constraint). Wider immediate constants
		 * would be truncated at assemble-time (e.g. "i" constraint). The  outb
		 * %al, %dx encoding is the only option for all other cases. %1 expands
		 * to %dx because  port  is a u16.  %w1 could be used if we had the port
		 * number a wider C type */
	}
};
