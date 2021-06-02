#pragma once

#include <drivers/io.h>
#include <sys/myos.h>

struct Serial {
	enum Com : u16 {
		Port1 = 0x3F8, // this is our debugging port
		Port2 = 0x2F8
	};

	static Com CurrentPort;

	static void init();

	static void setPort(Com port) {
		CurrentPort = port;
	}

	static u8 received() {
		return IO::inb(CurrentPort + 5) & 0x1;
	}

	static u8 read() {
		while(received() == 0)
			;
		return IO::inb(CurrentPort);
	}

	static u8 isTransmitEmpty() {
		return IO::inb(CurrentPort + 5) & 0x20;
	}

	static void write(char c) {
		while(isTransmitEmpty() == 0)
			;
		IO::outb(CurrentPort, c);
	}
};
