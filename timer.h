#pragma once

#include "system.h"

struct Timer {
	static volatile u32 ticks;
	static u32          frequency;

	static void wait(u32 ticks);
	static void setFrequency(u16 hz);
	static void handler(Register *r);
	static void init();
};
