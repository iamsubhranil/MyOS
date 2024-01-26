#include <arch/x86/irq.h>
#include <drivers/io.h>
#include <drivers/keyboard.h>
#include <drivers/ps2.h>
#include <drivers/scancode2.h>
#include <drivers/terminal.h>
#include <sched/scheduler.h>
#include <sched/semaphore.h>

ScancodeHandler handler           = ScancodeHandler();
Semaphore       keyboardSemaphore = Semaphore();

char Keyboard::getCharacter(bool blockOnZero) {
	keyboardSemaphore.acquire();
	char nextChar = 0;
	while(blockOnZero && ((nextChar = handler.getNextASCII()) == 0))
		Scheduler::yield();
	keyboardSemaphore.release();
	return nextChar;
}

void Keyboard::handleKeyboard(Register *r) {
	(void)r;
	u8 byte = Asm::inb(0x60);
	if(handler.handleKey(byte))
		keyboardSemaphore.release(1, false);
}

void Keyboard::init(u8 deviceNum) {
	PS2::sendToDevice(deviceNum, 0xF0, true, 0x2);
	PS2::sendToDevice(deviceNum, 0xF0, true, 0);
	u8 scanCode = PS2::readFromDevice(deviceNum);
	Terminal::info("Current scancode: ", Terminal::Mode::HexOnce, scanCode,
	               "!");
}
