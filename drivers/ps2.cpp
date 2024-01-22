#include <arch/x86/asm.h>
#include <arch/x86/irq.h>
#include <drivers/keyboard.h>
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
	return !((readStatusRegister() >> 1) & 0x01);
}

bool PS2::readyForInputWithDelay() {
	u64 tsc = Asm::rdtsc();
	while((Asm::rdtsc() - tsc) < 1000000 || !readyForInput())
		;
	return readyForInput();
}

bool PS2::readyForOutput() {
	return readStatusRegister() & 0x01;
}

bool PS2::readyForOutputWithDelay() {
	u64 tsc = Asm::rdtsc();
	while((Asm::rdtsc() - tsc) < 1000000 || !readyForOutput())
		;
	return readyForOutput();
}

u8 PS2::readResponse() {
	while(!readyForOutput())
		;
	return Asm::inb(0x60);
}

void PS2::sendToDevice(u8 num, u8 command, bool hasData, u8 data) {
	if(num == 1) {
		sendCommand(0xD4);
	}
	if(!readyForInputWithDelay()) {
		Terminal::err("Unable to send command ", Terminal::Mode::HexOnce,
		              command, " to device #", num);
		return;
	}
	Asm::outb(0x60, command);
	if(hasData) {
		if(!readyForInputWithDelay()) {
			Terminal::err("Unable to send data ", Terminal::Mode::HexOnce, data,
			              " to device #", num);
			return;
		}
		if(num == 1) {
			sendCommand(0xD4);
			if(!readyForInputWithDelay()) {
				Terminal::err("Unable to send data ", Terminal::Mode::HexOnce,
				              data, " to device #", num);
				return;
			}
		}
		Asm::outb(0x60, data);
	}
	// wait for ACK
	if(!readyForOutputWithDelay() || readFromDevice(num) != 0xFA) {
		Terminal::err("Command ", Terminal::Mode::HexOnce, command,
		              " not ACKed by device #", num, "!");
		return;
	}
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

void PS2::initDevice(u8 num) {
	u8 enable   = 0xAE;
	u8 portTest = 0xAB;
	u8 irq      = 1;
	if(num == 1) {
		enable   = 0xA8;
		portTest = 0xA9;
		irq      = 12;
	}
	PROMPT_INIT("PS2 - Device", Blue);
	PROMPT("Performing port test for device #", num, "..");
	sendCommand(portTest);
	u8 status = readResponse();
	if(status == 0x00) {
		PROMPT("Device #", num, " port test passed!");
	} else {
		PROMPT("Device #", num, " port test FAILED!");
		return;
	}
	PROMPT("Enabling device #", num, "..");
	sendCommand(enable);
	PROMPT("Sending reset to device #", num, "..");
	sendToDevice(num, 0xFF);
	status = readFromDevice(num);
	if(status != 0xAA) {
		Terminal::err("Device #", num, " reset failed!");
		return;
	}
	PROMPT("Device #", num, " reset successful!");
	// some devices may send additional data after reset, read those now
	if(PS2::readyForOutput())
		readFromDevice(num);

	PROMPT("Sending DISABLE_SCAN to device #", num, "..");
	sendToDevice(num, 0xF5);
	PROMPT("Sending IDENTIFY to device #", num, "..");
	sendToDevice(num, 0xF2);
	u8 type = readFromDevice(num);
	if(type == 0x0) {
		PROMPT("Identified device #", num, " as Standard PS/2 mouse!");
	} else if(type == 0xAB || type == 0xAC) {
		u8 oldtype = type;
		type       = readFromDevice(num);
		PROMPT("Device type 0xAB ", Terminal::Mode::HexOnce, type, "!");
		if(oldtype == 0xAB && type == 0x83) {
			PROMPT("Keyboard found!");
			PROMPT("Installing keyboard handler..");
			Keyboard::init(num);
			IRQ::installHandler(irq, Keyboard::handleKeyboard);
		}
	} else
		PROMPT("Device type ", Terminal::Mode::HexOnce, type, "!");
	PROMPT("Sending ENABLE_SCAN to device #", num, "..");
	sendToDevice(num, 0xF4);
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
	PROMPT("Current configuration: ", Terminal::Mode::HexOnce, currentConfig);
	// disable interrupts for now, and disable translation
	currentConfig &= 0xb4;
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
	currentConfig = readConfiguration();
	PROMPT("Current device configuration: ", Terminal::Mode::HexOnce,
	       currentConfig, "!");
	// determine if there are actually two channels
	if(hasSecondChannel) {
		PROMPT("Determining second channel..");
		sendCommand(0xA8);
		response = readConfiguration();
		if(response & 0x20) {
			hasSecondChannel = false;
			PROMPT("Second channel is non-functional!");
		} else {
			PROMPT("Second channel is functional!");
		}
		sendCommand(0xA7);
	}

	// enable devices
	PROMPT("Enabling devices..");
	initDevice(0);
	PROMPT("Disabling device #0..");
	sendCommand(0xAD);

	if(hasSecondChannel)
		initDevice(1);

	// enable device 0
	PROMPT("Enabling device #0 back..");
	sendCommand(0xAE);
	// enable interrupts
	PROMPT("Enabling device interrupts..");
	currentConfig = readConfiguration();
	currentConfig |= 0x03; // enable first two bits
	sendCommand(0x60, currentConfig);

	Terminal::done("PS2 initialization complete!");
}
