#include "terminal.h"
#include "string.h"
#include "vga.h"

Terminal::Terminal() {
	row    = 0;
	column = 0;
	color  = VGA::color(VGA::Color::LightGrey, VGA::Color::Black);
	buffer = (u16 *)0xB8000;
	for(u16 y = 0; y < VGA::Height; y++) {
		for(u16 x = 0; x < VGA::Width; x++) {
			const u16 index = y * VGA::Width + x;
			buffer[index]   = VGA::entry(' ', color);
		}
	}
}

void Terminal::setColor(u8 c) {
	color = c;
}

void Terminal::putEntryAt(char c, u8 color, u16 x, u16 y) {
	const u16 index = y * VGA::Width + x;
	buffer[index]   = VGA::entry(c, color);
}

char Terminal::getEntryFrom(u16 x, u16 y) {
	return buffer[(y * VGA::Width) + x];
}

void Terminal::moveUpOneRow() {
	u16 j = 1, i = 0;
	while(j < VGA::Height) {
		i = 0;
		while(i < VGA::Width) {
			putEntryAt(getEntryFrom(i, j), color, i, (j - 1));
			i++;
		}
		j++;
	}
	i = 0;
	while(i < VGA::Width) {
		putEntryAt(' ', color, i, VGA::Height - 1);
		i++;
	}
	row    = VGA::Height - 1;
	column = 0;
}

void Terminal::putchar(char c) {
	if(c == '\n') {
		column = VGA::Width - 1;
	} else if(c == '\r') {
		column = 0;
	} else if(c == '\b') {
		if(column == 0) {
			column = VGA::Width - 2;
			row--;
		} else
			column -= 2;
	} else {
		putEntryAt(c, color, column, row);
	}
	if(c != '\r') {
		if(++column == VGA::Width) {
			column = 0;
			if(++row == VGA::Height) {
				// row = 0;
				moveUpOneRow();
			}
		}
	}
}

void Terminal::write(const char *data, siz size) {
	for(u16 i = 0; i < size; i++) putchar(data[i]);
}

void Terminal::write(const char *data) {
	write(data, strlen(data));
}
