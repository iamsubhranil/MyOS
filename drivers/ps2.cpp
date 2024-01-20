#include <arch/x86/asm.h>
#include <drivers/ps2.h>
#include <drivers/terminal.h>

bool PS2::hasSecondChannel = false;

void PS2::sendCommand(u8 command) {
	Asm::outb(0x64, command);
}

void PS2::sendCommand(u8 command, u8 data) {
	sendCommand(command);
	while(!readyForInput())
		;
	Asm::outb(0x60, data);
}

u8 PS2::readStatusRegister() {
	return Asm::inb(0x64);
}

bool PS2::readyForInput() {
	// check if bit 1 is clear
	return ~(readStatusRegister() & 0x02);
}

bool PS2::readyForOutput() {
	return readStatusRegister() & 0x01;
}

u8 PS2::readResponse() {
	while(!readyForOutput())
		;
	return Asm::inb(0x60);
}

void PS2::sendToDevice(u8 num, u8 command) {
	if(num == 1) {
		sendCommand(0xD4);
	}
	u64 tsc = Asm::rdtsc();
	while((Asm::rdtsc() - tsc) < 1000000 || !readyForInput())
		;
	if(!readyForInput()) {
		Terminal::err("Unable to send command ", Terminal::Mode::HexOnce,
		              command, " to device #", num);
		return;
	}
	Asm::outb(0x60, command);
}

u8 PS2::readFromDevice(u8 num) {
	(void)num;
	while(!readyForOutput())
		;
	return Asm::inb(0x60);
}

u8 PS2::readConfiguration() {
	sendCommand(0x20);
	return readResponse();
}

void PS2::init() {
	PROMPT_INIT("PS2", Orange);
	PROMPT("Initializing PS2 controller..");

	// disable devices
	PROMPT("Disabling all devices..");
	sendCommand(0xAD);
	sendCommand(0xA7);
	// flush buffers
	PROMPT("Flushing buffers..");
	Asm::inb(0x60);
	// reconfig controller
	PROMPT("Reading controller configuration..");
	u8 currentConfig = readConfiguration();
	// 10111100
	// disable interrupts for now, and disable translation
	currentConfig &= 0xAB;
	if(currentConfig & 0x20) {
		PROMPT("Dual channel controller found!");
		hasSecondChannel = true;
	}
	// write back modified config
	PROMPT("Reconfiguring controller..");
	sendCommand(0x60, currentConfig);
	// perform self test
	PROMPT("Performing self test..");
	sendCommand(0xAA);
	u8 response = readResponse();
	if(response == 0x55) {
		PROMPT("Self test successful!");
	} else {
		PROMPT("Self test FAILED!");
	}
	// Re-write back modified config
	PROMPT("Rewriting controller configuration..");
	sendCommand(0x60, currentConfig);
	// determine if there are actually two channels
	if(hasSecondChannel) {
		PROMPT("Determining second channel..");
		sendCommand(0xA8);
		response = readConfiguration();
		if(response & 0x10) {
			hasSecondChannel = false;
			PROMPT("Second channel is non-functional!");
		} else {
			PROMPT("Second channel is functional!");
		}
		sendCommand(0xA7);
	}
	// perform interface tests
	PROMPT("Performing interface tests..");
	sendCommand(0xAB);
	u8 result = readResponse();
	if(result == 0x00) {
		PROMPT("First port test passed!");
	}
	sendCommand(0xA9);
	result = readResponse();
	if(result == 0x00) {
		PROMPT("Second port test passed!");
	}
	// enable devices
	PROMPT("Enabling device #0..");
	sendCommand(0xAE);
	PROMPT("Sending reset to device #0..");
	sendToDevice(0, 0xFF);
	u8 ack = readFromDevice(0);
	if(ack == 0xFA) {
		u8 status = readFromDevice(0);
		if(status == 0xAA) {
			PROMPT("Device #0 reset successful!");
		} else
			PROMPT("Device #0 reset FAILED!");
	} else {
		PROMPT("Device #0 reset NACK!");
	}
	PROMPT("Disabling device #0..");
	sendCommand(0xAD);

	PROMPT("Enabling device #1..");
	sendCommand(0xA8);
	PROMPT("Sending reset to device #1..");
	sendToDevice(1, 0xFF);
	ack = readFromDevice(1);
	if(ack == 0xFA) {
		u8 status = readFromDevice(1);
		if(status == 0xAA) {
			PROMPT("Device #1 reset successful!");
		} else
			PROMPT("Device #1 reset FAILED!");
	} else {
		PROMPT("Device #1 reset NACK!");
	}
	// enable device 0
	PROMPT("Enabling device #0 back..");
	sendCommand(0xAE);
	// enable interrupts
	PROMPT("Enabling device interrupts..");
	currentConfig |= 02; // enable first two bits
	sendCommand(0x60, currentConfig);
	Terminal::done("PS2 initialization complete!");
}
