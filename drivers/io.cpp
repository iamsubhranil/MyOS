#include <drivers/io.h>

u8 IO::getScanCode() {
	u8 c = 0;
	do {
		if(inb(0x60) != c) {
			c = inb(0x60);
			if(c > 0)
				return c;
		}
	} while(1);
}
