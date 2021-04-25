#pragma once

#include "system.h"

struct ISR {
	typedef void (*Routine)(Register *r);

	static Routine     routines[32];
	static const char *interruptMessages[32];

	static void installHandler(u8 isr, Routine r);
	static void uninstallHandler(u8 isr);
};
