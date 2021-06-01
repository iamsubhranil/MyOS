#include "terminal.h"
#include "asm.h"
#include "kernel_layout.h"
#include "multiboot.h"
#include "scopedlock.h"
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
SpinLock       Terminal::spinlock     = SpinLock();

void Terminal::init() {
	spinlock = SpinLock();
	row      = 0;
	column   = 0;
	color    = VGA::color(VGA::Color::LightGrey, VGA::Color::Black);
	buffer   = (u16 *)P2V(0xB8000);
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

u32 Terminal::write(const char &c) {
	ScopedLock sl(spinlock);
	switch(c) {
		case '\n': column = VGA::Width - 1; break;
		case '\r': column = 0; return 1; // we don't want to adjust anything
		case '\b': {
			if(column == 0) {
				column = VGA::Width - 2;
				row--;
			} else
				column -= 2;
		} break;
		case '\t':
			write_nolock(' ');
			write_nolock(' ');
			write_nolock(' ');
			return write_nolock(' ');
		default: putEntryAt(c, color, column, row); break;
	}
	if(++column == VGA::Width) {
		column = 0;
		if(++row == VGA::Height) {
			// row = 0;
			moveUpOneRow();
		}
	}
	return 1;
}

u32 Terminal::write_nolock(const char &c) {
	switch(c) {
		case '\n': column = VGA::Width - 1; break;
		case '\r': column = 0; return 1; // we don't want to adjust anything
		case '\b': {
			if(column == 0) {
				column = VGA::Width - 2;
				row--;
			} else
				column -= 2;
		} break;
		case '\t':
			write_nolock(' ');
			write_nolock(' ');
			write_nolock(' ');
			return write_nolock(' ');
		default: putEntryAt(c, color, column, row); break;
	}
	if(++column == VGA::Width) {
		column = 0;
		if(++row == VGA::Height) {
			// row = 0;
			moveUpOneRow();
		}
	}
	return 1;
}

u32 Terminal::writebytes(const char *const &data, siz size) {
	ScopedLock sl(spinlock);
	for(u16 i = 0; i < size; i++) write_nolock(data[i]);
	return size;
}

u32 Terminal::write(const char *const &data) {
	return writebytes(data, strlen(data));
}

u32 Terminal::write_dec(const u64 &value) {
	if(value == 0) {
		write('0');
		return 1;
	}
	char str[20] = {'0'};
	u8   start   = 20;
	u64  bak     = value;
	while(bak > 0) {
		--start;
		u8 dig     = bak % 10;
		str[start] = '0' + dig;
		bak /= 10;
	}
	return writebytes(&str[start], 20 - start);
}

u32 Terminal::write_hex(const u64 &value) {
	Terminal::write("0x");
	if(value == 0) {
		Terminal::write('0');
		return 3;
	}
	char str[16] = {'0'};
	u8   start   = 16;
	u64  bak     = value;
	while(bak > 0) {
		--start;
		u8 last = bak & 0xf;
		if(last < 10)
			str[start] = '0' + last;
		else
			str[start] = 'A' + (last - 10);
		bak >>= 4;
	}
	return writebytes(&str[start], 16 - start) + 2;
}

u32 Terminal::write_bin(const u64 &value) {
	Terminal::write("0b");
	if(value == 0) {
		Terminal::write('0');
		return 3;
	}
	char str[64] = {'0'};
	u8   start   = 64;
	u64  bak     = value;
	while(bak > 0) {
		--start;
		str[start] = '0' + (bak & 1);
		bak >>= 1;
	}
	return writebytes(&str[start], 64 - start) + 2;
}

u32 Terminal::write(const u64 &value) {
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

u32 Terminal::write(const i64 &value) {
	i64 bak = value;
	u8  add = 0;
	if(value < 0) {
		Terminal::write('-');
		bak = -value;
		add = 1;
	}
	return write((u64)bak) + add;
}

u32 Terminal::write(VGA::Color c) {
	ScopedLock sl(spinlock);
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
	ScopedLock sl(spinlock);
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
	ScopedLock sl(spinlock);
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
	ScopedLock sl(spinlock);
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
