#include "task.h"
#include "asm.h"
#include "irq.h"
#include "memory.h"
#include "terminal.h"
#include "timer.h"

extern "C" {
extern uptr eip_load();
extern void performTaskSwitch(uptr ebx, uptr edx, uptr esi, uptr ebp, uptr esp,
                              uptr physAddr, uptr eip);
}

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

void Task::performTaskSwitch(volatile Task *t) {
	::performTaskSwitch(t->regs.ebx, t->regs.edi, t->regs.esi, t->regs.ebp,
	                    t->regs.esp, t->pageDirectory->physicalAddr,
	                    t->regs.eip);
}

u32 Task::NextPid = 0;

Task::Task() {
	id    = NextPid++;
	next  = NULL;
	state = State::Scheduled;
	memset(&regs, 0, sizeof(Register));
	regs.fs = regs.es = regs.ds = regs.gs = 0x10;
}

void Task::run() {
	runner();
}

void Task::begin() {
	Task *t;
	asm volatile("mov %%esi, %0" : "=g"(t));
	asm volatile("sti");
	t->runner();
}
