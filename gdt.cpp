#include "gdt.h"
#include "myos.h"
#include "terminal.h"

extern "C" {
GDT::Pointer __gdtptr;
extern void  gdtFlush();
};

GDT::Entry GDT::entries[3] = {{0, 0, 0, 0, 0, 0}};

/* Setup a descriptor in the Global Descriptor Table */
void GDT::setGate(u32 num, u64 base, u64 limit, u8 access, u8 gran) {
	/* Setup the descriptor base address */
	entries[num].base_low    = (base & 0xFFFF);
	entries[num].base_middle = (base >> 16) & 0xFF;
	entries[num].base_high   = (base >> 24) & 0xFF;

	/* Setup the descriptor limits */
	entries[num].limit_low   = (limit & 0xFFFF);
	entries[num].granularity = ((limit >> 16) & 0x0F);

	/* Finally, set up the granularity and access flags */
	entries[num].granularity |= (gran & 0xF0);
	entries[num].access = access;
}

void GDT::init() {
	Terminal::info("Setting up GDT..");
	/* Setup the GDT pointer and limit */
	__gdtptr.limit = (sizeof(GDT::Entry) * 3) - 1;
	__gdtptr.base  = entries;

	Terminal::prompt(VGA::Color::Brown, "GDT", "Setting NULL gate..");
	/* Our NULL descriptor */
	setGate(0, 0, 0, 0, 0);

	Terminal::prompt(VGA::Color::Brown, "GDT", "Setting code segment..");
	/* The second entry is our Code Segment. The base address
	 *  is 0, the limit is 4GBytes, it uses 4KByte granularity,
	 *  uses 32-bit opcodes, and is a Code Segment descriptor.
	 *  Please check the table above in the tutorial in order
	 *  to see exactly what each value means */
	setGate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

	Terminal::prompt(VGA::Color::Brown, "GDT", "Setting data segment..");
	/* The third entry is our Data Segment. It's EXACTLY the
	 *  same as our code segment, but the descriptor type in
	 *  this entry's access byte says it's a Data Segment */
	setGate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

	Terminal::prompt(VGA::Color::Brown, "GDT", "Installing changes..");
	/* Flush out the old GDT and install the new changes! */
	gdtFlush();

	Terminal::done("GDT is set up successfully!");
}
