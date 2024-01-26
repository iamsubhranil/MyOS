#pragma once

#include <sys/myos.h>

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

	static inline u8 inb(volatile u16 port) {
		volatile u8 ret;
		asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
		return ret;
	}

	static inline void outb(volatile u16 port, volatile u8 val) {
		asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
		/* There's an outb %al, $imm8  encoding, for compile-time constant port
		 * numbers that fit in 8b.  (N constraint). Wider immediate constants
		 * would be truncated at assemble-time (e.g. "i" constraint). The  outb
		 * %al, %dx encoding is the only option for all other cases. %1 expands
		 * to %dx because  port  is a u16.  %w1 could be used if we had the port
		 * number a wider C type */
	}

	static inline void invlpg(uptr address) {
		asm volatile("invlpg (%0)" ::"r"(address) : "memory");
	}

	static inline void cr3_store(uptr address) {
		asm volatile("mov %0, %%cr3" ::"r"(address));
	}

	static inline void disableInterrupt(uptr &flags) {
		asm volatile("pushf\n"
		             "pop %0\n"
		             "cli"
		             : "=g"(flags));
	}

	static inline void restoreInterrupt(uptr flags) {
		asm volatile("push %0\n"
		             "popf"
		             :
		             : "g"(flags));
	}

	static inline u64 rdtsc() {
		u64 ret;
		asm volatile("rdtsc" : "=A"(ret));
		return ret;
	}
};

#define PAUSEI()          \
	uptr __current_flags; \
	Asm::disableInterrupt(__current_flags);
#define RESUMEI() Asm::restoreInterrupt(__current_flags);
