#pragma once

#include <drivers/io.h>
#include <sys/myos.h>

struct Serial {
	enum Com : u16 {
		Port1 = 0x3F8, // this is our debugging port
		Port2 = 0x2F8
	};

	// should be in the order of Terminal::Color
	static constexpr const char *AsciiColors[] = {
	    "\u001b[30m",   "\u001b[31m",   "\u001b[32m",   "\u001b[33m",
	    "\u001b[34m",   "\u001b[35m",   "\u001b[36m",   "\u001b[37m",
	    "\u001b[30;1m", "\u001b[31;1m", "\u001b[32;1m", "\u001b[33;1m",
	    "\u001b[34;1m", "\u001b[35;1m", "\u001b[36;1m", "\u001b[37;1m",
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
