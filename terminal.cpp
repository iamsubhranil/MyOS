#include "terminal.h"
#include "string.h"
#include "vga.h"

u16        Terminal::row    = 0;
u16        Terminal::column = 0;
VGA::Color Terminal::color =
    VGA::color(VGA::Color::LightGrey, VGA::Color::Black);
u16 *Terminal::buffer = 0;

void Terminal::init() {
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

void Terminal::setColor(VGA::Color c) {
	color = c;
}

void Terminal::putEntryAt(u8 c, VGA::Color color, u16 x, u16 y) {
	const u16 index = y * VGA::Width + x;
	buffer[index]   = VGA::entry(c, color);
}

void Terminal::putEntryAt(u16 entry, u16 x, u16 y) {
	const u16 index = y * VGA::Width + x;
	buffer[index]   = entry;
}

u16 Terminal::getEntryFrom(u16 x, u16 y) {
	return buffer[(y * VGA::Width) + x];
}

void Terminal::moveUpOneRow() {
	u16 j = 1, i = 0;
	while(j < VGA::Height) {
		i = 0;
		while(i < VGA::Width) {
			putEntryAt(getEntryFrom(i, j), i, (j - 1));
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

void Terminal::write(u64 value) {
	u64 maxshift = 1;
	while(value / maxshift) {
		maxshift *= 10;
	}
	maxshift /= 10;
	while(maxshift > 0) {
		u8 val = '0' + ((value / maxshift));
		putchar(val);
		value = value % maxshift;
		maxshift /= 10;
	}
}

void Terminal::prompt(VGA::Color foreground, const char *prompt,
                      const char *message) {
	write("[ ", 2);
	VGA::Color oldColor = color;
	setColor(foreground);
	write(prompt);
	setColor(oldColor);
	write(" ] ", 3);
	write(message);
	write("\n", 1);
}

void Terminal::info(const char *data) {
	prompt(VGA::Color::Cyan, "Info", data);
}

void Terminal::warn(const char *data) {
	prompt(VGA::Color::Magenta, "Warn", data);
}

void Terminal::done(const char *data) {
	prompt(VGA::Color::Green, "Done", data);
}

void Terminal::fail(const char *data) {
	prompt(VGA::Color::Red, "Fail", data);
}

void Terminal::err(const char *data) {
	prompt(VGA::Color::Red, "ERR ", data);
}
