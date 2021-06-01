#ifndef KEYCODES_H
#define KEYCODES_H

#include <sys/myos.h>

struct Keycodes {
	static const char *getKey(u8 scancode);
	static const char *getSecondKey(u8 firstScancode);
};

#endif
