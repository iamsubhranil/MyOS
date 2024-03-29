#include <arch/x86/asm.h>
#include <arch/x86/gdt.h>
#include <arch/x86/idt.h>
#include <arch/x86/irq.h>
#include <arch/x86/kernel_layout.h>
#include <boot/multiboot.h>
#include <drivers/io.h>
#include <drivers/keyboard.h>
#include <drivers/ps2.h>
#include <drivers/terminal.h>
#include <drivers/timer.h>
#include <mem/memory.h>
#include <mem/paging.h>
#include <misc/shell.h>
#include <sched/scheduler.h>
#include <sched/task.h>
#include <sys/stacktrace.h>
#include <sys/syscall.h>

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
	Terminal t;
	t.write("syscall is working!\n");
	return 0;
}

void writeSomething(u32 id, u32 a, u32 b, u32 c, u32 d, u32 e, u32 f, u32 g) {
	u32 res = 1;
	Terminal::write("Starting task ", id, "..\n");
	Terminal::write("Args are: ", a, " ", b, " ", c, " ", d, " ", e, " ", f,
	                " ", g, " \n");
	for(;;) {
		res++;
		if(res % ((id + 1) * 5000000) == 0) {
			Terminal::write("Thread: ", id, " yielding..\n");
			Scheduler::yield();
			// Terminal::write("Thread: ", id, " resumed..\n");
			// Terminal::write("Thread: ", id, "\tresult: ", res, "\n");
		}
		// Asm::sti();
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

void sleepTask() {
	for(u32 i = 1; i < 1000; i++) {
		u32 ms = 5000;
		Terminal::write("[Sleeper] Sleeping for ", ms / 1000, " sec..\n");
		Scheduler::sleep(ms);
		Terminal::write("[Sleeper] Woke up!\n");
	}
}

void addNewTask() {
	for(u32 i = 1; i < 25; i++) {
		Scheduler::submit(writeSomething, i, i * 2, i * 4, i * 8, i * 16,
		                  i * 32, i * 64, i * 128);
	}
}

void loopTask() {
	Terminal::write("Starting loop..");
	writeSomething(1, 2, 3, 4, 5, 6, 7, 8);
}

u32 fib(u32 val) {
	// u32 id  = Scheduler::CurrentTask->id;
	u32 res = val;
	if(val > 2) {
		Future<u32> *left  = Scheduler::submit(fib, val - 1);
		Future<u32> *right = Scheduler::submit(fib, val - 2);

		// Terminal::write("Thread: ", id, " fib(", val, "): waiting for ",
		//                val - 1, " & ", val - 2, "\n");
		res = left->get() + right->get();
		// Terminal::write("Thread: ", id, " fib(", val, "): received ", val -
		// 1,
		//                " & ", val - 2, "\n");
	}
	// Terminal::write("Thread: ", id, " fib(", val, "): returning ", res,
	// "\n");
	return res;
}

void calcFib(u32 id) {
	// Terminal::write("Starting fib: ", id, "\n");
	u32 res = fib(10);
	// Terminal::write("Ending fib: ", id, "\n");
	u32 finalId = Task::NextPid;
	Terminal::write("FIB RESULT: Thread ", id, ": ", res,
	                "\tTasks Created: ", finalId - id, "\n");
}

void addFibTask() {
	for(u32 i = 1; i < 25; i++) {
		Scheduler::submit(calcFib, i);
	}
}

void addNewTaskNoArg() {
	for(u32 i = 1; i < 5; i++) {
		Scheduler::submit(loopTask);
	}
}

void finishableTask() {
	Terminal::write("In finishable task..\n");
}

// entry-point
void kernelMain(Multiboot *mboot, uptr stack_, uptr useless0, uptr useless1) {
	(void)stack_;
	(void)useless0;
	(void)useless1;

	// Terminal::CurrentOutput = Terminal::Output::VGA;
	Terminal::init(mboot);
	mboot->dump();
	// reinit paging
	Paging::init(mboot);
	// disable interrupts before setting up gdt, idt and irqs
	Asm::cli();
	GDT::init();
	IDT::init();
	IRQ::init();
	// since irq0 is already hooked to scheduling tasks,
	// enable the scheduler before enabling interrupts.
	// the scheduler will set itself up, and then enable
	// interrupt itself.
	Scheduler::init();
	PS2::init();

#ifdef DEBUG
	for(i32 i = 0; i < 100; i++) {
		Terminal::write("Line: ", i, "\n");
	}
	u8 a[] = {1, 2, 3};
	u8 b[] = {2, 3, 4};
	Terminal::write((i32)memcmp(a, b, 3));
	Terminal::write("a\nb\nc\nd\ne\nf\ng\n");
	while(1) {
		u32 *a = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(Terminal::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)a, "..");
		u32 *b = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(Terminal::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)b, "..");
		Terminal::prompt(Terminal::Color::Blue, "Kernel",
		                 "Releasing the u32s..");
		Memory::free(a);
		Memory::free(b);
		u32 *c = (u32 *)Memory::alloc(sizeof(u32));
		Terminal::prompt(Terminal::Color::Blue, "Kernel", "u32 allocated at ",
		                 Terminal::Mode::HexOnce, (void *)c, "..");
		Memory::free(c);
		Terminal::prompt(Terminal::Color::Blue, "Kernel",
		                 "Releasing the u32..");
		break;
	}
	Terminal::write("\n");
	Terminal::write(Terminal::Mode::Dec);
	Scheduler::submit(finishableTask);
	addFibTask();
	Scheduler::submit(sleepTask);
#endif
	Shell::init();
	Scheduler::submit(Shell::run);
	asm volatile("1: hlt; jmp 1b");
}

#if defined(__cplusplus)
} /* Use C linkage for kernel_main. */
#endif
