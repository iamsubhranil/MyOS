#include "terminal.h"
#include "string.h"
#include "vga.h"

u16        Terminal::row    = 0;
u16        Terminal::column = 0;
VGA::Color Terminal::color =
    VGA::color(VGA::Color::LightGrey, VGA::Color::Black);
VGA::Color Terminal::previousColor =
    VGA::color(VGA::Color::LightGrey, VGA::Color::Black);
Terminal::Mode Terminal::currentMode  = Terminal::Mode::Dec;
Terminal::Mode Terminal::previousMode = Terminal::Mode::Dec;
u16 *          Terminal::buffer       = 0;

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
	previousColor = color;
	color         = c;
}

void Terminal::setMode(Terminal::Mode c) {
	previousMode = currentMode;
	currentMode  = c;
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

u32 Terminal::write(char c) {
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
	return 1;
}

u32 Terminal::writebytes(const char *data, siz size) {
	for(u16 i = 0; i < size; i++) write(data[i]);
	return size;
}

u32 Terminal::write(const char *data) {
	return writebytes(data, strlen(data));
}

u32 Terminal::write_dec(u64 value) {
	if(value == 0) {
		write('0');
		return 1;
	}
	char str[20] = {'0'};
	u8   start   = 20;
	while(value > 0) {
		--start;
		u8 dig     = value % 10;
		str[start] = '0' + dig;
		value /= 10;
	}
	return writebytes(&str[start], 20 - start);
}

u32 Terminal::write_hex(u64 value) {
	Terminal::write("0x");
	if(value == 0) {
		Terminal::write('0');
		return 3;
	}
	char str[16] = {'0'};
	u8   start   = 16;
	while(value > 0) {
		--start;
		u8 last = value & 0xf;
		if(last < 10)
			str[start] = '0' + last;
		else
			str[start] = 'A' + (last - 10);
		value >>= 4;
	}
	return writebytes(&str[start], 16 - start) + 2;
}

u32 Terminal::write_bin(u64 value) {
	Terminal::write("0b");
	if(value == 0) {
		Terminal::write('0');
		return 3;
	}
	char str[64] = {'0'};
	u8   start   = 64;
	while(value > 0) {
		--start;
		str[start] = '0' + (value & 1);
		value >>= 1;
	}
	return writebytes(&str[start], 64 - start) + 2;
}

u32 Terminal::write(u64 value) {
	u32 ret;
	switch(currentMode) {
		case Terminal::Mode::Dec: ret = write_dec(value); break;
		case Terminal::Mode::DecOnce:
			ret = write_dec(value);
			setMode(previousMode);
			break;
		case Terminal::Mode::Hex: ret = write_hex(value); break;
		case Terminal::Mode::HexOnce:
			ret = write_hex(value);
			setMode(previousMode);
			break;
		case Terminal::Mode::Bin: ret = write_bin(value); break;
		case Terminal::Mode::BinOnce:
			ret = write_bin(value);
			setMode(previousMode);
			break;
		default: return 0;
	}
	return ret;
}

u32 Terminal::write(i64 value) {
	u8 add = 0;
	if(value < 0) {
		Terminal::write('-');
		value = -value;
		add   = 1;
	}
	return write((u64)value) + add;
}

u32 Terminal::write(VGA::Color c) {
	switch(c) {
		case VGA::Color::Reset: {
			setColor(previousColor);
		} break;
		default: {
			setColor(c);
		} break;
	}
	return 0;
}

u32 Terminal::write(Terminal::Mode m) {
	switch(m) {
		case Terminal::Mode::Reset: {
			setMode(previousMode);
		} break;
		default: {
			setMode(m);
		} break;
	}
	return 0;
}

u32 Terminal::write(Terminal::Move m) {
	switch(m) {
		case Terminal::Move::Up: {
			if(row == 0) {
				row = VGA::Height - 1;
			} else {
				row--;
			}
		} break;
		case Terminal::Move::Down: {
			row = (row + 1) % VGA::Height;
		} break;
		case Terminal::Move::Left: {
			if(column == 0) {
				column = VGA::Width - 1;
			} else {
				column--;
			}
		} break;
		case Terminal::Move::Right: {
			column = (column + 1) % VGA::Width;
		} break;
	}
	return 0;
}

u32 Terminal::write(Terminal::Control c) {
	switch(c) {
		case Terminal::Control::ClearLine: {
			for(u32 i = 0; i < VGA::Width; i++) {
				putEntryAt(' ', row, i);
				column = 0;
			}
		} break;
	}
	return 0;
}
