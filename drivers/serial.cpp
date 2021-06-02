#include <drivers/serial.h>

Serial::Com Serial::CurrentPort = Serial::Com::Port1;

void Serial::init() {
	IO::outb(CurrentPort + 1, 0x00); // Disable all interrupts
	IO::outb(CurrentPort + 3, 0x80); // Enable DLAB (set baud rate divisor)
	IO::outb(CurrentPort + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	IO::outb(CurrentPort + 1, 0x00); //                  (hi byte)
	IO::outb(CurrentPort + 3, 0x03); // 8 bits, no parity, one stop bit
	IO::outb(CurrentPort + 2,
	         0xC7); // Enable FIFO, clear them, with 14-byte threshold
	IO::outb(CurrentPort + 4, 0x0B); // IRQs enabled, RTS/DSR set
	IO::outb(CurrentPort + 4,
	         0x1E); // Set in loopback mode, test the serial chip
	IO::outb(CurrentPort + 0, 0xAE); // Test serial chip (send byte 0xAE and
	                                 // check if serial returns same byte)

	// Check if serial is faulty (i.e: not same byte as sent)
	// if(inb(CurrentPort + 0) != 0xAE) {
	// 	return 1;
	// }

	// If serial is not faulty set it in normal operation mode
	// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
	IO::outb(CurrentPort + 4, 0x0F);
	// return 0;
}
