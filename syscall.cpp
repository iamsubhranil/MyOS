#include "syscall.h"
#include "isr.h"
#include "terminal.h"

#define SYSCALL0(x) extern int x();
#define SYSCALL1(x, y) extern int x(y p1);
#define SYSCALL2(x, y, z) extern int x(y p1, z p2);
#define SYSCALL3(x, y, z, a) extern int x(y p1, z p2, a p3);
#define SYSCALL4(x, y, z, a, b) extern int x(y p1, z p2, a p3, b p4);
#define SYSCALL5(x, y, z, a, b, c) extern int x(y p1, z p2, a p3, b p4, c p5);
#include "syscall_list.h"

static void *SyscallImpl[] = {
#define SYSCALL0(x) (void *)&x,
#define SYSCALL1(x, y) (void *)&x,
#define SYSCALL2(x, y, z) (void *)&x,
#define SYSCALL3(x, y, z, a) (void *)&x,
#define SYSCALL4(x, y, z, a, b) (void *)&x,
#define SYSCALL5(x, y, z, a, b, c) (void *)&x,
#include "syscall_list.h"
};

void Syscall::handleSyscall(Register *regs) {
	// Firstly, check if the requested syscall number is valid.
	// The syscall number is found in EAX.
	if(regs->eax >= (u32)Id::__Invalid) {
		Terminal::err("Invalid syscall number: ", regs->eax, "!\n");
		for(;;)
			;
		return;
	}

	// Get the required syscall location.
	void *location = SyscallImpl[regs->eax];

	// We don't know how many parameters the function wants, so we just
	// push them all onto the stack in the correct order. The function will
	// use all the parameters it wants, and we can pop them all back off
	// afterwards.
	int ret;
	asm volatile(" \
     push %1\n\
     push %2\n\
     push %3\n\
     push %4\n\
     push %5\n\
     call *%6\n\
     pop %%ebx\n\
     pop %%ebx\n\
     pop %%ebx\n\
     pop %%ebx\n\
     pop %%ebx;"
	             : "=a"(ret)
	             : "r"(regs->edi), "r"(regs->esi), "r"(regs->edx),
	               "r"(regs->ecx), "r"(regs->ebx), "r"(location));
	regs->eax = ret;
}

void Syscall::init() {
	ISR::installHandler(InterruptNumber, handleSyscall);
}
