#pragma once

#include <sys/system.h>

struct PS2 {
	static bool hasSecondChannel;

	static void init();

	static void sendCommand(u8 command);
	static void sendCommand(u8 command, u8 data);

	static u8 readStatusRegister();
	static u8 readResponse();
	static u8 readConfiguration();

	static void sendToDevice(u8 num, u8 command, bool hasData = false,
	                         u8 data = 0);
	static u8   readFromDevice(u8 num);

	static bool readyForInput();
	static bool readyForInputWithDelay();
	static bool readyForOutput();
	static bool readyForOutputWithDelay();

	static void initDevice(u8 num);
};
