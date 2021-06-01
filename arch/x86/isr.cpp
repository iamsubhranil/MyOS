#include <arch/x86/isr.h>
#include <drivers/terminal.h>
/*
Exception # 	    Description 	            Error Code?
    0 	    Division By Zero Exception 	            No
    1 	    Debug Exception 	                    No
    2 	    Non Maskable Interrupt Exception 	    No
    3 	    Breakpoint Exception 	                No
    4 	    Into Detected Overflow Exception 	    No
    5 	    Out of Bounds Exception 	            No
    6 	    Invalid Opcode Exception        	    No
    7 	    No Coprocessor Exception 	            No
    8 	    Double Fault Exception 	                Yes
    9 	    Coprocessor Segment Overrun Exception 	No
    10 	    Bad TSS Exception 	                    Yes
    11 	    Segment Not Present Exception 	        Yes
    12 	    Stack Fault Exception 	                Yes
    13 	    General Protection Fault Exception 	    Yes
    14 	    Page Fault Exception 	                Yes
    15 	    Unknown Interrupt Exception 	        No
    16 	    Coprocessor Fault Exception 	        No
    17 	    Alignment Check Exception (486+) 	    No
    18 	    Machine Check Exception (Pentium/586+) 	No
    19 to 31 	Reserved Exceptions 	            No

As mentioned earlier, some exceptions push an error code onto the stack.
To decrease the complexity, we handle this by pushing a dummy error code
of 0 onto the stack for any ISR that doesn't push an error code already.
This keeps a uniform stack frame. To track which exception is firing,
we also push the interrupt number on the stack.

*/
const char *ISR::interruptMessages[255] = {
    "Division By Zero Exception",
    "Debug Exception",
    "Non Maskable Interrupt Exception",
    "Breakpoint Exception",
    "Into Detected Overflow Exception",
    "Out of Bounds Exception",
    "Invalid Opcode Exception",
    "No Coprocessor Exception",
    "Double Fault Exception",
    "Coprocessor Segment Overrun Exception",
    "Bad TSS Exception",
    "Segment Not Present Exception",
    "Stack Fault Exception",
    "General Protection Fault Exception",
    "Page Fault Exception",
    "Unknown Interrupt Exception",
    "Coprocessor Fault Exception",
    "Alignment Check Exception (486+)",
    "Machine Check Exception (Pentium/586+)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};

ISR::Routine ISR::routines[255] = {NULL};

void ISR::installHandler(u8 isr, ISR::Routine r) {
	routines[isr] = r;
}

void ISR::uninstallHandler(u8 isr) {
	routines[isr] = NULL;
}

extern "C" {
void _fault_handler(Register *registers) {
	Terminal::info("INTERRUPT");
	/* Is this a fault whose number is from 0 to 31? */
	if(registers->int_no < 256) {
		/* Display the description for the Exception that occurred.
		 *  In this tutorial, we will simply halt the system using an
		 *  infinite loop */
		if(ISR::interruptMessages[registers->int_no]) {
			Terminal::prompt(VGA::Color::Magenta, "INTRHW",
			                 ISR::interruptMessages[registers->int_no]);
		} else {
			Terminal::prompt(VGA::Color::Magenta, "INTRHW",
			                 Terminal::Mode::HexOnce, registers->int_no);
		}
		if(ISR::routines[registers->int_no]) {
			ISR::routines[registers->int_no](registers);
		} else {
			Terminal::prompt(VGA::Color::Magenta, "INTRHW",
			                 "No interrupt handler found! Looping!");
			for(;;)
				;
		}
	}
}
};
