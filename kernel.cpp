#include "asm.h"
#include "gdt.h"
#include "idt.h"
#include "io.h"
#include "irq.h"
#include "keycodes.h"
#include "memory.h"
#include "multiboot.h"
#include "paging.h"
#include "scheduler.h"
#include "syscall.h"
#include "task.h"
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

// syscall
extern int hello() {
	Terminal::write("syscall is working!\n");
	return 0;
}

void writeSomething() {
	u32 res = 1;
	for(;;) {
		Asm::cli();
		res++;
		if(res % 1000000 == 0)
			Terminal::write("\nThread: ", (u32)Scheduler::CurrentTask->id,
			                "\tresult: ", res);
		Asm::sti();
	}
	// u32 id = 1;
	// while(1) {
	// u32 id = Scheduler::CurrentTask->id;
	// Terminal::write("At task: ", id, " iteration:", iteration++, "\n");
	// }
	// Scheduler::unschedule();
}

#if defined(__cplusplus)
extern "C" { /* Use C linkage for kernel_main. */
#endif

void addNewTask() {
	for(u32 i = 0; i < 1; i++) {
		Task *t   = Memory::create<Task>();
		t->runner = writeSomething;
		Scheduler::schedule(t);
	}
}

// entry-point
void kernelMain(Multiboot *mboot, uptr stack_, uptr useless0, uptr useless1) {
	(void)stack_;
	(void)useless0;
	(void)useless1;

	// void kernelMain() {
	Terminal::init();
	mboot->dump();
	// reinit paging
	Paging::init(mboot);
	// disable interrupts before setting up gdt, idt and irqs
	Asm::cli();
	GDT::init();
	IDT::init();
	IRQ::init();
	// we are all done, now enable interrupts
	Asm::sti();
	Scheduler::init();
	// writeSomething();
	// Task::init();
	// Syscall::init();
	// Task::switchToUserMode(esp);
	while(1) {
		u32 *a = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(VGA::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)a, "..");
		u32 *b = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(VGA::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)b, "..");
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Releasing the u32s..");
		Memory::free(a);
		Memory::free(b);
		u32 *c = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(VGA::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)c, "..");
		Memory::free(c);
		Terminal::prompt(VGA::Color::Blue, "Kernel", "Releasing the u32..");
		// Timer::wait(Timer::frequency);
		// Terminal::prompt(VGA::Color::Blue, "Kernel", "Waited for 1 \
		// seconds!");
		break;
		// while(1)
		//	;
	}
	Terminal::write(Terminal::Mode::Dec);
	addNewTask();
	writeSomething();
	// asm volatile("1: jmp 1b");
}

#if defined(__cplusplus)
} /* Use C linkage for kernel_main. */
#endif
