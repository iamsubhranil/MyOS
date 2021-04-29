#include "asm.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "irq.h"
#include "keycodes.h"
#include "memory.h"
#include "paging.h"
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
	Asm::cli();
	GDT::init();
	IDT::init();
	IRQ::init();
	// we are all done, now enable interrupts
	Asm::sti();
	Paging::init();
	Timer::init();
	while(1) {
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Allocating an u32..");
		u32 *a = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(VGA::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)a, "..");
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Allocating an u32..");
		u32 *b = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(VGA::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)b, "..");
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Releasing the u32s..");
		Memory::free(a);
		Memory::free(b);
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Allocating an u32..");
		u32 *c = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(VGA::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)c, "..");
		Terminal::prompt(VGA::Color::Blue, "Kernel",
		                 "Trying to page fault at 0xA0000000..");
		u8 *x             = (u8 *)(0xA0000000);
		u8  do_page_fault = *x;
		Timer::wait(do_page_fault);
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Waited for 1 seconds!");
	}
}

#if defined(__cplusplus)
} /* Use C linkage for kernel_main. */
#endif
