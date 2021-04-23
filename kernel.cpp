#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "irq.h"
#include "keycodes.h"
#include "terminal.h"
#include "timer.h"

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
	Terminal::init();
	// disable interrupts before setting up gdt, idt and irqs
	asm("cli");
	GDT::init();
	IDT::init();
	IRQ::init();
	// we are all done, now enable interrupts
	asm("sti");
	Timer::init();
	while(1) {
		Timer::wait(Timer::frequency);
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Waited for 1 seconds!");
	}
}

#if defined(__cplusplus)
} /* Use C linkage for kernel_main. */
#endif
