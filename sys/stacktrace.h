#pragma once

#include <boot/multiboot.h>
#include <sys/myos.h>

struct Stacktrace {
	struct StackFrame {
		StackFrame *next;
		uptr        eip;
	};

	struct Symbol {
		u32 ip;
		u32 stridx; // index to the string table
	};

	static void dumpSymbols();
	static void loadSymbols(Multiboot *boot);
	static void print(void *ebp = NULL);
	static void print(uptr ebp) {
		union {
			uptr  a;
			void *b;
		} converter;
		converter.a = ebp;
		print(converter.b);
	}
};
