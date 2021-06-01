#include "task.h"
#include "asm.h"
#include "irq.h"
#include "memory.h"
#include "terminal.h"
#include "timer.h"

Task::StateSegment Task::taskStateSegment = {0, 0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0, 0,
                                             0, 0, 0, 0, 0, 0, 0, 0, 0};

void Task::switchToUserMode(u32 kernelStackPointer) {
	setKernelStack(kernelStackPointer); // ????
	// Set up a stack structure for switching to user mode.
	asm volatile("  \
            cli\n\
            mov $0x23, %ax\n\
            mov %ax, %ds\n\
            mov %ax, %es\n\
            mov %ax, %fs\n\
            mov %ax, %gs\n\
            mov %esp, %eax\n\
            pushl $0x23\n\
            pushl %eax\n\
            pushf\n\
            pushl $0x1B\n\
            push $1f\n\
            pop %eax /* Get EFLAGS back into EAX. The only way to read EFLAGS is to pushf then pop. */ \n\
            or $0x200, %eax /* Set the IF flag */ \n \
            push %eax /* Push the new EFLAGS value back onto the stack. */ \n \
            iret\n\
        1: \
     ");
}

void Task::setKernelStack(uptr stack_) {
	taskStateSegment.esp0 = stack_;
}

u32 Task::NextPid = 0;

Task::Task() {
	id   = NextPid++;
	prev = next = NULL;
	nextInList  = NULL;
	state       = State::New;
	memset(&regs, 0, sizeof(Register));
	regs.fs = regs.es = regs.ds = regs.gs = 0x10;
	regs.cs                               = 0x08;
}
