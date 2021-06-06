#include <arch/x86/descriptor.h>
#include <arch/x86/gdt.h>
#include <drivers/terminal.h>
#include <sched/task.h>

extern "C" {
GDT::Pointer __gdtptr;
extern void  gdtFlush();
extern void  tssFlush();
};

GDT::Entry GDT::entries[6] = {{0, 0, 0, 0, 0, 0}};

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
	PROMPT_INIT("GDT", Orange);
	/* Setup the GDT pointer and limit */
	__gdtptr.limit = sizeof(GDT::entries) - 1;
	__gdtptr.base  = (uptr)entries;

	PROMPT("Setting NULL gate..");
	/* Our NULL descriptor */
	setGate(0, 0, 0, 0, 0);

	PROMPT("Setting kernel code segment..");
	/* The second entry is our Code Segment. The base address
	 *  is 0, the limit is 4GBytes, it uses 4KByte granularity,
	 *  uses 32-bit opcodes, and is a Code Segment descriptor.
	 *  Please check the table above in the tutorial in order
	 *  to see exactly what each value means */
	setGate(1, 0, 0xFFFFFFFF,

	        Descriptor::accessByte(
	            Descriptor::PriviledgeLevel::Ring0,
	            Descriptor::Type::CodeOrData, Descriptor::Executable::Yes,
	            Descriptor::Direction::Up, Descriptor::ReadWrite::Allowed),

	        Descriptor::flagByte(Descriptor::Granularity::KiloByte,
	                             Descriptor::OperandSize::Bits32));

	PROMPT("Setting kernel data segment..");
	/* The third entry is our Data Segment. It's EXACTLY the
	 *  same as our code segment, but the descriptor type in
	 *  this entry's access byte says it's a Data Segment */
	setGate(2, 0, 0xFFFFFFFF,

	        Descriptor::accessByte(
	            Descriptor::PriviledgeLevel::Ring0,
	            Descriptor::Type::CodeOrData, Descriptor::Executable::No,
	            Descriptor::Direction::Up, Descriptor::ReadWrite::Allowed),

	        Descriptor::flagByte(Descriptor::Granularity::KiloByte,
	                             Descriptor::OperandSize::Bits32));

	PROMPT("Setting user code segment..");
	setGate(3, 0, 0xFFFFFFFF,
	        Descriptor::accessByte(
	            Descriptor::PriviledgeLevel::Ring3,
	            Descriptor::Type::CodeOrData, Descriptor::Executable::Yes,
	            Descriptor::Direction::Up, Descriptor::ReadWrite::Allowed),

	        Descriptor::flagByte(Descriptor::Granularity::KiloByte,
	                             Descriptor::OperandSize::Bits32));

	PROMPT("Setting user data segment..");
	setGate(4, 0, 0xFFFFFFFF,
	        Descriptor::accessByte(
	            Descriptor::PriviledgeLevel::Ring3,
	            Descriptor::Type::CodeOrData, Descriptor::Executable::No,
	            Descriptor::Direction::Up, Descriptor::ReadWrite::Allowed),

	        Descriptor::flagByte(Descriptor::Granularity::KiloByte,
	                             Descriptor::OperandSize::Bits32));

	PROMPT("Setting task state segment..");
	Task::taskStateSegment.init(0x01, 0x0);
	setGate(5, (uptr)&Task::taskStateSegment,
	        (uptr)&Task::taskStateSegment + sizeof(Task::StateSegment), 0xE9,
	        0x0);

	PROMPT("Installing changes..");
	/* Flush out the old GDT and install the new changes! */
	gdtFlush();
	PROMPT("Installing TSS..");
	tssFlush();

	Terminal::done("GDT is set up successfully!");
}
