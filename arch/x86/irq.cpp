#include <arch/x86/idt.h>
#include <arch/x86/irq.h>
#include <drivers/io.h>
#include <drivers/terminal.h>
/* These are own ISRs that point to our special IRQ handler
 *  instead of the regular 'fault_handler' function */
extern "C" {
extern void _irq0();
extern void _irq1();
extern void _irq2();
extern void _irq3();
extern void _irq4();
extern void _irq5();
extern void _irq6();
extern void _irq7();
extern void _irq8();
extern void _irq9();
extern void _irq10();
extern void _irq11();
extern void _irq12();
extern void _irq13();
extern void _irq14();
extern void _irq15();
};

IRQ::Routine IRQ::routines[16] = {NULL};

void IRQ::installHandler(u8 num, IRQ::Routine r) {
	routines[num] = r;
}

void IRQ::uninstallHandler(u8 num) {
	routines[num] = NULL;
}

/*  Normally, IRQs 0 to 7 are mapped to entries 8 to 15. This
 *  is a problem in protected mode, because IDT entry 8 is a
 *  Double Fault! Without remapping, every time IRQ0 fires,
 *  you get a Double Fault Exception, which is NOT actually
 *  what's happening. We send commands to the Programmable
 *  Interrupt Controller (PICs - also called the 8259's) in
 *  order to make IRQ0 to 15 be remapped to IDT entries 32 to
 *  47 */
void IRQ::remap() {
	IO::outb(0x20, 0x11);
	IO::outb(0xA0, 0x11);
	IO::outb(0x21, 0x20);
	IO::outb(0xA1, 0x28);
	IO::outb(0x21, 0x04);
	IO::outb(0xA1, 0x02);
	IO::outb(0x21, 0x01);
	IO::outb(0xA1, 0x01);
	IO::outb(0x21, 0x0);
	IO::outb(0xA1, 0x0);
}

void IRQ::init() {
	Terminal::info("Setting up IRQs..");
	/*  We first remap the interrupt controllers, and then we install
	 *  the appropriate ISRs to the correct entries in the IDT. This
	 *  is just like installing the exception handlers */
	Terminal::prompt(VGA::Color::Brown, "IRQ", "Remapping the PIC..");
	remap();

	Terminal::prompt(VGA::Color::Brown, "IRQ", "Setting up IRQ hooks..");
	IDT::setGate(32, (uptr)_irq0, 0x08, 0x8E);
	IDT::setGate(33, (uptr)_irq1, 0x08, 0x8E);
	IDT::setGate(34, (uptr)_irq2, 0x08, 0x8E);
	IDT::setGate(35, (uptr)_irq3, 0x08, 0x8E);
	IDT::setGate(36, (uptr)_irq4, 0x08, 0x8E);
	IDT::setGate(37, (uptr)_irq5, 0x08, 0x8E);
	IDT::setGate(38, (uptr)_irq6, 0x08, 0x8E);
	IDT::setGate(39, (uptr)_irq7, 0x08, 0x8E);
	IDT::setGate(40, (uptr)_irq8, 0x08, 0x8E);
	IDT::setGate(41, (uptr)_irq9, 0x08, 0x8E);
	IDT::setGate(42, (uptr)_irq10, 0x08, 0x8E);
	IDT::setGate(43, (uptr)_irq11, 0x08, 0x8E);
	IDT::setGate(44, (uptr)_irq12, 0x08, 0x8E);
	IDT::setGate(45, (uptr)_irq13, 0x08, 0x8E);
	IDT::setGate(46, (uptr)_irq14, 0x08, 0x8E);
	IDT::setGate(47, (uptr)_irq15, 0x08, 0x8E);
	Terminal::done("IRQs set up successfully!");
}

void IRQ::finishIrq(Register *r) {

	/* If the IDT entry that was invoked was greater than 40
	 *  (meaning IRQ8 - 15), then we need to send an EOI to
	 *  the slave controller */
	if(r->int_no >= 40) {
		IO::outb(0xA0, 0x20);
	}

	/* In either case, we need to send an EOI to the master
	 *  interrupt controller too */
	IO::outb(0x20, 0x20);
}

extern "C" {
/*  Each of the IRQ ISRs point to this function, rather than
 *  the 'fault_handler' in 'isr.cpp'. The IRQ Controllers need
 *  to be told when you are done servicing them, so you need
 *  to send them an "End of Interrupt" command (0x20). There
 *  are two 8259 chips: The first exists at 0x20, the second
 *  exists at 0xA0. If the second controller (an IRQ from 8 to
 *  15) gets an interrupt, you need to acknowledge the
 *  interrupt at BOTH controllers, otherwise, you only send
 *  an EOI command to the first controller. If you don't send
 *  an EOI, you won't raise any more IRQs */
void _irq_handler(Register *r) {
	/* Find out if we have a custom handler to run for this
	 *  IRQ, and then finally, run it */
	auto handler = IRQ::routines[r->int_no - 32];
	if(handler) {
		handler(r);
	}
	IRQ::finishIrq(r);
}

void _irq0_test() {
	Terminal::info("here");
}
}

