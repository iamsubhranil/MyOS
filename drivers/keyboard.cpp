#include <arch/x86/irq.h>
#include <drivers/io.h>
#include <drivers/keyboard.h>
#include <drivers/terminal.h>
#include <ds/queue.h>

Queue<char> characterQueue = Queue<char>();

const char *Keyboard::getKey(u8 c) {
	switch(c) {
		case 0x01: return "escape pressed";
		case 0x02: return "1 pressed";
		case 0x03: return "2 pressed";
		case 0x04: return "3 pressed";
		case 0x05: return "4 pressed";
		case 0x06: return "5 pressed";
		case 0x07: return "6 pressed";
		case 0x08: return "7 pressed";
		case 0x09: return "8 pressed";
		case 0x0A: return "9 pressed";
		case 0x0B: return "0 (zero) pressed";
		case 0x0C: return "- pressed";
		case 0x0d: return "= pressed";
		case 0x0E: return "backspace pressed";
		case 0x0F: return "tab pressed";
		case 0x10: return "Q pressed";
		case 0x11: return "W pressed";
		case 0x12: return "E pressed";
		case 0x13: return "R pressed";
		case 0x14: return "T pressed";
		case 0x15: return "Y pressed";
		case 0x16: return "U pressed";
		case 0x17: return "I pressed";
		case 0x18: return "O pressed";
		case 0x19: return "P pressed";
		case 0x1A: return "[ pressed";
		case 0x1B: return "] pressed";
		case 0x1C: return "enter pressed";
		case 0x1d: return "left control pressed";
		case 0x1E: return "A pressed";
		case 0x1F: return "S pressed";
		case 0x20: return "D pressed";
		case 0x21: return "F pressed";
		case 0x22: return "G pressed";
		case 0x23: return "H pressed";
		case 0x24: return "J pressed";
		case 0x25: return "K pressed";
		case 0x26: return "L pressed";
		case 0x27: return "; pressed";
		case 0x28: return "' (single quote) pressed";
		case 0x29: return "` (back tick) pressed";
		case 0x2A: return "left shift pressed";
		case 0x2B: return "\\ pressed";
		case 0x2C: return "Z pressed";
		case 0x2d: return "X pressed";
		case 0x2E: return "C pressed";
		case 0x2F: return "V pressed";
		case 0x30: return "B pressed";
		case 0x31: return "N pressed";
		case 0x32: return "M pressed";
		case 0x33: return ": pressed";
		case 0x34: return ". pressed";
		case 0x35: return "/ pressed";
		case 0x36: return "right shift pressed";
		case 0x37: return "(keypad) * pressed";
		case 0x38: return "left alt pressed";
		case 0x39: return "space pressed";
		case 0x3A: return "CapsLock pressed";
		case 0x3B: return "F1 pressed";
		case 0x3C: return "F2 pressed";
		case 0x3d: return "F3 pressed";
		case 0x3E: return "F4 pressed";
		case 0x3F: return "F5 pressed";
		case 0x40: return "F6 pressed";
		case 0x41: return "F7 pressed";
		case 0x42: return "F8 pressed";
		case 0x43: return "F9 pressed";
		case 0x44: return "F10 pressed";
		case 0x45: return "NumberLock pressed";
		case 0x46: return "ScrollLock pressed";
		case 0x47: return "(keypad) 7 pressed";
		case 0x48: return "(keypad) 8 pressed";
		case 0x49: return "(keypad) 9 pressed";
		case 0x4A: return "(keypad) - pressed";
		case 0x4B: return "(keypad) 4 pressed";
		case 0x4C: return "(keypad) 5 pressed";
		case 0x4d: return "(keypad) 6 pressed";
		case 0x4E: return "(keypad) + pressed";
		case 0x4F: return "(keypad) 1 pressed";
		case 0x50: return "(keypad) 2 pressed";
		case 0x51: return "(keypad) 3 pressed";
		case 0x52: return "(keypad) 0 pressed";
		case 0x53: return "(keypad) . pressed";
		case 0x57: return "F11 pressed";
		case 0x58: return "F12 pressed";
		case 0x81: return "escape released";
		case 0x82: return "1 released";
		case 0x83: return "2 released";
		case 0x84: return "3 released";
		case 0x85: return "4 released";
		case 0x86: return "5 released";
		case 0x87: return "6 released";
		case 0x88: return "7 released";
		case 0x89: return "8 released";
		case 0x8A: return "9 released";
		case 0x8B: return "0 (zero) released";
		case 0x8C: return "- released";
		case 0x8d: return "= released";
		case 0x8E: return "backspace released";
		case 0x8F: return "tab released";
		case 0x90: return "Q released";
		case 0x91: return "W released";
		case 0x92: return "E released";
		case 0x93: return "R released";
		case 0x94: return "T released";
		case 0x95: return "Y released";
		case 0x96: return "U released";
		case 0x97: return "I released";
		case 0x98: return "O released";
		case 0x99: return "P released";
		case 0x9A: return "[ released";
		case 0x9B: return "] released";
		case 0x9C: return "enter released";
		case 0x9d: return "left control released";
		case 0x9E: return "A released";
		case 0x9F: return "S released";
		case 0xA0: return "D released";
		case 0xA1: return "F released";
		case 0xA2: return "G released";
		case 0xA3: return "H released";
		case 0xA4: return "J released";
		case 0xA5: return "K released";
		case 0xA6: return "L released";
		case 0xA7: return "; released";
		case 0xA8: return "' (single quote) released";
		case 0xA9: return "` (back tick) released";
		case 0xAA: return "left shift released";
		case 0xAB: return "\\ released";
		case 0xAC: return "Z released";
		case 0xAd: return "X released";
		case 0xAE: return "C released";
		case 0xAF: return "V released";
		case 0xB0: return "B released";
		case 0xB1: return "N released";
		case 0xB2: return "M released";
		case 0xB3: return ": released";
		case 0xB4: return ". released";
		case 0xB5: return "/ released";
		case 0xB6: return "right shift released";
		case 0xB7: return "(keypad) * released";
		case 0xB8: return "left alt released";
		case 0xB9: return "space released";
		case 0xBA: return "CapsLock released";
		case 0xBB: return "F1 released";
		case 0xBC: return "F2 released";
		case 0xBd: return "F3 released";
		case 0xBE: return "F4 released";
		case 0xBF: return "F5 released";
		case 0xC0: return "F6 released";
		case 0xC1: return "F7 released";
		case 0xC2: return "F8 released";
		case 0xC3: return "F9 released";
		case 0xC4: return "F10 released";
		case 0xC5: return "NumberLock released";
		case 0xC6: return "ScrollLock released";
		case 0xC7: return "(keypad) 7 released";
		case 0xC8: return "(keypad) 8 released";
		case 0xC9: return "(keypad) 9 released";
		case 0xCA: return "(keypad) - released";
		case 0xCB: return "(keypad) 4 released";
		case 0xCC: return "(keypad) 5 released";
		case 0xCd: return "(keypad) 6 released";
		case 0xCE: return "(keypad) + released";
		case 0xCF: return "(keypad) 1 released";
		case 0xD0: return "(keypad) 2 released";
		case 0xD1: return "(keypad) 3 released";
		case 0xD2: return "(keypad) 0 released";
		case 0xD3: return "(keypad) . released";
		case 0xD7: return "F11 released";
		case 0xD8: return "F12 released";
		case 0xE0: {
			c = IO::getScanCode();
			return getSecondKey(c);
		}
			/*	case 0x2A:
			     case 0x37: return "print screen pressed";
			     case 0xB7:
			     case 0xAA: return "print screen released";
			     case 0xE1:
			     case 0x1D:
			     case 0x45:
			     case 0xE1:
			     case 0x9D:
			     case 0xC5: return "pause pressed";
			     */
		default: return "Unknown key!";
	}
}

const char *Keyboard::getSecondKey(u8 c) {
	switch(c) {
		case 0x1C: return "(keypad) enter pressed";
		case 0x1d: return "right control pressed";
		case 0x35: return "(keypad) / pressed";
		case 0x38: return "right alt (or altGr) pressed";
		case 0x47: return "home pressed";
		case 0x48: return "cursor up pressed";
		case 0x49: return "page up pressed";
		case 0x4B: return "cursor left pressed";
		case 0x4d: return "cursor right pressed";
		case 0x4F: return "end pressed";
		case 0x50: return "cursor down pressed";
		case 0x51: return "page down pressed";
		case 0x52: return "insert pressed";
		case 0x53: return "delete pressed";
		case 0x5B: return "left GUI pressed";
		case 0x5C: return "right GUI pressed";
		case 0x5d: return "apps pressed";
		case 0x9C: return "(keypad) enter released";
		case 0x9d: return "right control released";
		case 0xB5: return "(keypad) / released";
		case 0xB8: return "right alt (or altGr) released";
		case 0xC7: return "home released";
		case 0xC8: return "cursor up released";
		case 0xC9: return "page up released";
		case 0xCB: return "cursor left released";
		case 0xCd: return "cursor right released";
		case 0xCF: return "end released";
		case 0xD0: return "cursor down released";
		case 0xD1: return "page down released";
		case 0xD2: return "insert released";
		case 0xD3: return "delete released";
		case 0xDB: return "left GUI released";
		case 0xDC: return "right GUI released";
		case 0xDd: return "apps released";
		default: return "Unknown key!";
	}
}

void Keyboard::handleKeyboard(Register *r) {
	(void)r;
	u8 byte = Asm::inb(0x60);
	Terminal::info("Read: ", Terminal::Mode::HexOnce, byte, "\n");
}

void Keyboard::init() {
}
