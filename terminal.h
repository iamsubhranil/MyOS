#pragma once

#include "myos.h"
#include "vga.h"

struct Terminal {
	static u16        row;
	static u16        column;
	static VGA::Color color;
	static u16 *      buffer;

	static void init();

	static void setColor(VGA::Color color);
	static void putEntryAt(u8 c, VGA::Color color, u16 x, u16 y);
	static void putEntryAt(u16 entry, u16 x, u16 y);
	static u16  getEntryFrom(u16 x, u16 y);
	static void moveUpOneRow();
	static void putchar(char c);
	static void write(const char *data, siz len);
	static void write(const char *data);
	static void write(u64 value);

	static void prompt(VGA::Color foreground, const char *prompt,
	                   const char *message);
	static void info(const char *data);
	static void warn(const char *data);
	static void done(const char *data);
	static void fail(const char *data);
	static void err(const char *data);
};
