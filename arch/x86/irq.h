#pragma once

#include <sys/myos.h>
#include <sys/system.h>

struct IRQ {
	typedef void (*Routine)(Register *r);

	static Routine routines[16];

	static void installHandler(u8 num, Routine routine);
	static void uninstallHandler(u8 num);
	static void remap();
	static void init();
	// let the controller know we acknowledged it
	static void finishIrq(Register *r);
};
