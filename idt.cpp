#include "idt.h"

extern "C" {
IDT::Pointer __idtptr;
extern void  idtFlush();
}

IDT::Entry IDT::entries[256] = {{0, 0, 0, 0, 0}};

/* Use this function to set an entry in the IDT. Alot simpler
 *  than twiddling with the GDT ;) */
void IDT::setGate(u8 num, u64 base, u16 sel, u8 flags) {
	/* The interrupt routine's base address */
	entries[num].base_lo = (base & 0xFFFF);
	entries[num].base_hi = (base >> 16) & 0xFFFF;

	/* The segment or 'selector' that this IDT entry will use
	 *  is set here, along with any access flags */
	entries[num].sel     = sel;
	entries[num].always0 = 0;
	entries[num].flags   = flags;
}

/* Installs the IDT */
void IDT::init() {
	/* Sets the special IDT pointer up, just like in 'gdt.c' */
	__idtptr.limit = (sizeof(IDT::Entry) * 256) - 1;
	__idtptr.base  = entries;

	/* IDT is already cleared out to all 0's */

	/* Add any new ISRs to the IDT here using idt_set_gate */

	/* Points the processor's internal register to the new IDT */
	idtFlush();
}
