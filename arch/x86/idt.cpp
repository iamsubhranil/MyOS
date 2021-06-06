#include <arch/x86/asm.h>
#include <arch/x86/idt.h>
#include <drivers/terminal.h>
#include <sys/syscall.h>

extern "C" {
u8 __isr_syscall_no = Syscall::InterruptNumber;

// isr stubs written in boot.s
extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();
extern void _isr_syscall();
}

IDT::Entry   IDT::entries[256] = {{0, 0, 0, 0, 0}};
IDT::Pointer IDT::__idtptr     = {0, 0};

/* Use this function to set an entry in the IDT. Alot simpler
 *  than twiddling with the GDT ;) */
void IDT::setGate(u8 num, uptr base, u16 sel, u8 flags) {
	/* The interrupt routine's base address */
	entries[num].base_lo = (base & 0xFFFF);
	entries[num].base_hi = (base >> 16) & 0xFFFF;

	/* The segment or 'selector' that this IDT entry will use
	 *  is set here, along with any access flags */
	entries[num].sel     = sel;
	entries[num].always0 = 0;
	entries[num].flags   = flags | 0x60;
}

/* Installs the IDT */
void IDT::init() {
	PROMPT_INIT("IDT", Orange);

	PROMPT("Initializing interrupt table vector..");
	/* Sets the special IDT pointer up, just like in 'gdt.c' */
	__idtptr.limit = sizeof(IDT::entries) - 1;
	__idtptr.base  = (uptr)entries;

	/* IDT is already cleared out to all 0's */

	PROMPT("Setting up interrupt hooks..");
	/*  Add any new ISRs to the IDT here using IDT::setGate */
	/*  We set the access
	 *  flags to 0x8E. This means that the entry is present, is
	 *  running in ring 0 (kernel level), and has the lower 5 bits
	 *  set to the required '14' (for interrupt gate), which is
	 *  represented by 'E' in hex. */
	setGate(0, (uptr)_isr0, 0x08, 0x8E);
	setGate(1, (uptr)_isr1, 0x08, 0x8E);
	setGate(2, (uptr)_isr2, 0x08, 0x8E);
	setGate(3, (uptr)_isr3, 0x08, 0x8E);
	setGate(4, (uptr)_isr4, 0x08, 0x8E);
	setGate(5, (uptr)_isr5, 0x08, 0x8E);
	setGate(6, (uptr)_isr6, 0x08, 0x8E);
	setGate(7, (uptr)_isr7, 0x08, 0x8E);
	setGate(8, (uptr)_isr8, 0x08, 0x8E);
	setGate(9, (uptr)_isr9, 0x08, 0x8E);
	setGate(10, (uptr)_isr10, 0x08, 0x8E);
	setGate(11, (uptr)_isr11, 0x08, 0x8E);
	setGate(12, (uptr)_isr12, 0x08, 0x8E);
	setGate(13, (uptr)_isr13, 0x08, 0x8E);
	setGate(14, (uptr)_isr14, 0x08, 0x8E);
	setGate(15, (uptr)_isr15, 0x08, 0x8E);
	setGate(16, (uptr)_isr16, 0x08, 0x8E);
	setGate(17, (uptr)_isr17, 0x08, 0x8E);
	setGate(18, (uptr)_isr18, 0x08, 0x8E);
	setGate(19, (uptr)_isr19, 0x08, 0x8E);
	setGate(20, (uptr)_isr20, 0x08, 0x8E);
	setGate(21, (uptr)_isr21, 0x08, 0x8E);
	setGate(22, (uptr)_isr22, 0x08, 0x8E);
	setGate(23, (uptr)_isr23, 0x08, 0x8E);
	setGate(24, (uptr)_isr24, 0x08, 0x8E);
	setGate(25, (uptr)_isr25, 0x08, 0x8E);
	setGate(26, (uptr)_isr26, 0x08, 0x8E);
	setGate(27, (uptr)_isr27, 0x08, 0x8E);
	setGate(28, (uptr)_isr28, 0x08, 0x8E);
	setGate(29, (uptr)_isr29, 0x08, 0x8E);
	setGate(30, (uptr)_isr30, 0x08, 0x8E);
	setGate(31, (uptr)_isr31, 0x08, 0x8E);
	setGate(Syscall::InterruptNumber, (uptr)_isr_syscall, 0x08, 0x8E);

	PROMPT("Installing changes..");
	/* Points the processor's internal register to the new IDT */
	Asm::lidt(__idtptr);
	Terminal::done("IDT is set up successfully!");
}
