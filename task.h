#pragma once

#include "myos.h"
#include "paging.h"
#include "string.h"

struct Task {
	static void switchToUserMode(u32 kernelStackPointer);

	// A struct describing a Task State Segment.
	struct StateSegment {
		StateSegment *prev_tss; // The previous TSS - if we used hardware task
		                        // switching this would form a linked list.
		uptr esp0; // The stack pointer to load when we change to kernel mode.
		u32  ss0;  // The stack segment to load when we change to kernel mode.
		u32  esp1; // Unused...
		u32  ss1;
		u32  esp2;
		u32  ss2;
		u32  cr3;
		u32  eip;
		u32  eflags;
		u32  eax;
		u32  ecx;
		u32  edx;
		u32  ebx;
		u32  esp;
		u32  ebp;
		u32  esi;
		u32  edi;
		u32  es;  // The value to load into ES when we change to kernel mode.
		u32  cs;  // The value to load into CS when we change to kernel mode.
		u32  ss;  // The value to load into SS when we change to kernel mode.
		u32  ds;  // The value to load into DS when we change to kernel mode.
		u32  fs;  // The value to load into FS when we change to kernel mode.
		u32  gs;  // The value to load into GS when we change to kernel mode.
		u32  ldt; // Unused...
		u16  trap;
		u16  iomap_base;

		void init(u32 kernelStackSegment, u32 kernelStackPointer) {
			// set everything to 0
			memset(this, 0, sizeof(StateSegment));

			ss0  = kernelStackSegment;
			esp0 = kernelStackPointer;

			// Here we set the cs, ss, ds, es, fs and gs entries in the TSS.
			// These specify what
			// segments should be loaded when the processor switches to kernel
			// mode. Therefore they are just our normal kernel code/data
			// segments - 0x08 and 0x10 respectively, but with the last two bits
			// set, making 0x0b and 0x13. The setting of these bits sets the RPL
			// (requested privilege level) to 3, meaning that this TSS can be
			// used to switch to kernel mode from ring 3.
			cs = 0x0b;
			ss = ds = es = fs = gs = 0x13;
		}
	};

	static StateSegment taskStateSegment;

	static void setKernelStack(uptr stack_);

	enum State { Scheduled, Ready, Waiting };

	static u32 NextPid;

	// properties of a task
	u32 id; // id of the task
	// uptr ebp, eip, esp;
	Register           regs;
	Paging::Directory *pageDirectory;
	Task *             next;
	State              state;
	typedef void (*Runner)();
	Runner runner;

	static const siz DefaultStackSize =
	    1024 * 1024 * 4; // let's make it 4KiB for now
	Task();
	// the first time run is called, it saves the
	// eip of the point from which runner() is started
	void        run();
	static void begin();
	// fork requires copying the old stack, which requires changing
	// ebp values, and that's a bit hackish. so we don't provide
	// that for now.
	// static u32  fork();   // creates a new task

	static void switch_(Register *r); // performs the task switch

	static void
	    performTaskSwitch(volatile Task *t); // it forgets where it came from
};
