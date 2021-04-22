#include "gdt.h"
#include "io.h"
#include "keycodes.h"
#include "terminal.h"

/* Check if the compiler thinks we are targeting the wrong operating system. */
#if defined(__linux__)
#error \
    "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

#if defined(__cplusplus)
extern "C" { /* Use C linkage for kernel_main. */
#endif

// entry-point
void kernelMain() {
	GDT::init();

	u8 c = IO::getScanCode();
	/* Initialize terminal interface */
	Terminal terminal;

	/* Newline support is left as an exercise. */
	terminal.write("\nEnter a key : \n");
	c       = IO::getScanCode();
	u8 oldc = 0x00;
	while(1) {
		if(oldc != c) {
			terminal.write(Keycodes::getKey(c));
			terminal.write("\n");
		}
		oldc = c;
		c    = IO::getScanCode();
		if(c == 0xE0)
			c = IO::getScanCode();
	}
}

#if defined(__cplusplus)
} /* Use C linkage for kernel_main. */
#endif
