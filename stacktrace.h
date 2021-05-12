#pragma once

#include "myos.h"

struct Stacktrace {
	struct StackFrame {
		StackFrame *next;
		uptr        eip;
	};

	static void print();
};
