#include <arch/x86/kernel_layout.h>
#include <drivers/font.h>
#include <drivers/serial.h>
#include <drivers/terminal.h>
#include <drivers/vga.h>
#include <sched/scopedlock.h>
#include <sys/string.h>

u16              Terminal::row           = 0;
u16              Terminal::column        = 0;
u16              Terminal::vgaLineWidth  = 0;
u16              Terminal::vgaLineHeight = 0;
Terminal::Color  Terminal::color         = Terminal::Color::White;
Terminal::Color  Terminal::previousColor = Terminal::Color::White;
Terminal::Mode   Terminal::currentMode   = Terminal::Mode::Dec;
Terminal::Mode   Terminal::previousMode  = Terminal::Mode::Dec;
u16 *            Terminal::buffer        = 0;
SpinLock         Terminal::spinlock      = SpinLock();
Terminal::Output Terminal::CurrentOutput =
    Terminal::Output::Serial; // default output is serial
bool Terminal::SerialInited = false;
bool Terminal::VGAInited    = false;

void Terminal::initVga(Multiboot *m) {
	// map the vbe framebuffer
	Multiboot::VbeModeInfo *vbe =
	    (Multiboot::VbeModeInfo *)P2V(m->vbe_mode_info);
	VGA::init(vbe);
	Font::init();
	vgaLineWidth  = Font::charactersPerLine();
	vgaLineHeight = Font::linesPerFrame();
	VGAInited     = true;
}

void Terminal::initSerial() {
	Serial::init();
	SerialInited = true;
}

void Terminal::init(Multiboot *m) {
	// if none of the modes have been inited, init the spinlock
	if(!SerialInited && !VGAInited)
		spinlock = SpinLock();
	ScopedLock sl(spinlock);
	switch(CurrentOutput) {
		case Output::Serial: initSerial(); break;
		case Output::VGA: initVga(m); break;
	}
}

void Terminal::setColor(Color c) {
	previousColor = color;
	color         = c;
}

void Terminal::setMode(Terminal::Mode c) {
	previousMode = currentMode;
	currentMode  = c;
}

void Terminal::writeSerialColor(Color c) {
	const char *col = Serial::AsciiColors[c >= Orange ? Red : c];
	while(*col++) Serial::write(*(col - 1));
}

void Terminal::putEntryAt(u8 c, Color color, u16 x, u16 y) {
	switch(CurrentOutput) {
		case Output::Serial: // serial output just prints and does nothing else
			writeSerialColor(color);
			Serial::write(c);
			break;
		case Output::VGA: {
			u16 starty = y * Font::CurrentFont.asciiHeight;
			u16 startx = x * Font::CurrentFont.maxWidth;
			Font::Renderer::render(VGA::Point(startx, starty), c,
			                       VGA::Colors[color]);
		} break;
	}
}

void Terminal::putEntryAt(u16 entry, u16 x, u16 y) {
	putEntryAt(entry, color, x, y);
}

void Terminal::moveUpOneRow() {
	// if current mode is serial, we can't move up one row
	if(CurrentOutput == Output::Serial)
		return;
	// VGA output, so move the first line to 0
	VGA::scrollForward(VGA::Point(0, Font::CurrentFont.asciiHeight));
}

u32 Terminal::write(const char &c) {
	ScopedLock sl(spinlock);
	return write_nolock(c);
}

u32 Terminal::write_nolock(const char &c) {
	// if we are writing to serial, we'll directly output the char
	if(CurrentOutput == Output::Serial) {
		putEntryAt(c, 0, 0);
		return 1;
	}
	// otherwise, we need to manipulate the screen space ourselves
	switch(c) {
		case '\n': column = Font::charactersPerLine() - 1; break;
		case '\r': column = 0; return 1; // we don't want to adjust anything
		case '\b': {
			if(column == 0) {
				column = vgaLineWidth - 2;
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
	if(++column == vgaLineWidth) {
		column = 0;
		if(++row == vgaLineHeight) {
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

u32 Terminal::write(Color c) {
	ScopedLock sl(spinlock);
	switch(c) {
		case Color::Reset: {
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
				row = vgaLineHeight - 1;
			} else {
				row--;
			}
		} break;
		case Terminal::Move::Down: {
			row = (row + 1) % vgaLineHeight;
		} break;
		case Terminal::Move::Left: {
			if(column == 0) {
				column = vgaLineWidth - 1;
			} else {
				column--;
			}
		} break;
		case Terminal::Move::Right: {
			column = (column + 1) % vgaLineWidth;
		} break;
	}
	return 0;
}

u32 Terminal::write(Terminal::Control c) {
	ScopedLock sl(spinlock);
	switch(c) {
		case Terminal::Control::ClearLine: {
			for(u32 i = 0; i < vgaLineWidth; i++) {
				putEntryAt(' ', row, i);
				column = 0;
			}
		} break;
	}
	return 0;
}

u32 Terminal::write(Terminal::Output o) {
	ScopedLock sl(spinlock);
	CurrentOutput = o;
	return 0;
}
