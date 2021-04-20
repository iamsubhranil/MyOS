#pragma once

#include "myos.h"

struct Terminal {
	u16  row;
	u16  column;
	u8   color;
	u16 *buffer;

	Terminal();

	void setColor(u8 color);
	void putEntryAt(char c, u8 color, u16 x, u16 y);
	char getEntryFrom(u16 x, u16 y);
	void moveUpOneRow();
	void putchar(char c);
	void write(const char *data, siz len);
	void write(const char *data);
};
