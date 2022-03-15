#pragma once

#include <sys/myos.h>

struct Stacktrace {
	struct StackFrame {
		StackFrame *next;
		uptr        eip;
	};

	static void print();
	static void print(void *ebp);
	static void print(uptr ebp) {
		union {
			uptr  a;
			void *b;
		} converter;
		converter.a = ebp;
		print(converter.b);
	}
};
