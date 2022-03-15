#pragma once

#include <sys/system.h>

struct Timer {
	static volatile u32 ticks;
	static u32          frequency;

	static void wait(u32 ticks);
	static void setFrequency(u32 hz);
	static void handler(Register *r);
	static void init();
	// returns the average increment in tsc
	// per ms.
	// should be called in an uninterruptable context.
	static u64 calibrateTSC();
};
