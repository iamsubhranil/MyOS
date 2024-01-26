#include <ds/staticqueue.h>

enum Key : u8 {
	Acpi_Power = 0, Acpi_Sleep = 1, Acpi_Wake = 2, Alpha_A = 3, Alpha_B = 4, 
	Alpha_C = 5, Alpha_D = 6, Alpha_E = 7, Alpha_F = 8, Alpha_G = 9, Alpha_H = 10, 
	Alpha_I = 11, Alpha_J = 12, Alpha_K = 13, Alpha_L = 14, Alpha_M = 15, 
	Alpha_N = 16, Alpha_O = 17, Alpha_P = 18, Alpha_Q = 19, Alpha_R = 20, 
	Alpha_S = 21, Alpha_T = 22, Alpha_U = 23, Alpha_V = 24, Alpha_W = 25, 
	Alpha_X = 26, Alpha_Y = 27, Alpha_Z = 28, Apps = 29, Backspace = 30, 
	Capslock = 31, Cursor_Down = 32, Cursor_Left = 33, Cursor_Right = 34, 
	Cursor_Up = 35, Delete = 36, End = 37, Enter = 38, Escape = 39, F1 = 40, 
	F10 = 41, F11 = 42, F12 = 43, F2 = 44, F3 = 45, F4 = 46, F5 = 47, F6 = 48, 
	F7 = 49, F8 = 50, F9 = 51, Home = 52, Insert = 53, Keypad_Enter = 54, 
	Left_Alt = 55, Left_Control = 56, Left_Gui = 57, Left_Shift = 58, 
	Multimedia_Calculator = 59, Multimedia_Email = 60, Multimedia_Media_Select = 61, 
	Multimedia_Mute = 62, Multimedia_My_Computer = 63, Multimedia_Next_Track = 64, 
	Multimedia_Play_Pause = 65, Multimedia_Previous_Track = 66, 
	Multimedia_Stop = 67, Multimedia_Volume_Down = 68, Multimedia_Volume_Up = 69, 
	Multimedia_Www_Back = 70, Multimedia_Www_Favourites = 71, 
	Multimedia_Www_Forward = 72, Multimedia_Www_Home = 73, 
	Multimedia_Www_Refresh = 74, Multimedia_Www_Search = 75, 
	Multimedia_Www_Stop = 76, Num_0 = 77, Num_1 = 78, Num_2 = 79, Num_3 = 80, 
	Num_4 = 81, Num_5 = 82, Num_6 = 83, Num_7 = 84, Num_8 = 85, Num_9 = 86, 
	Num_Keypad_0 = 87, Num_Keypad_1 = 88, Num_Keypad_2 = 89, Num_Keypad_3 = 90, 
	Num_Keypad_4 = 91, Num_Keypad_5 = 92, Num_Keypad_6 = 93, Num_Keypad_7 = 94, 
	Num_Keypad_8 = 95, Num_Keypad_9 = 96, Numberlock = 97, Page_Down = 98, 
	Page_Up = 99, Pause = 100, Print_Screen = 101, Right_Alt = 102, 
	Right_Control = 103, Right_Gui = 104, Right_Shift = 105, Scrolllock = 106, 
	Space = 107, Sym_Backslash = 108, Sym_Backtick = 109, Sym_Comma = 110, 
	Sym_Dot = 111, Sym_Equals = 112, Sym_Forwardslash = 113, 
	Sym_Keypad_Backslash = 114, Sym_Keypad_Dot = 115, Sym_Keypad_Minus = 116, 
	Sym_Keypad_Plus = 117, Sym_Keypad_Star = 118, Sym_Minus = 119, 
	Sym_Semicolon = 120, Sym_Singlequote = 121, Sym_Squareclose = 122, 
	Sym_Squareopen = 123, Tab = 124, };


struct ScancodeHandler {
	StaticQueue<Key, 1024> keys;
	u64 modifierStates;
	u8 lastLevelCode;
	ScancodeHandler() : keys(), modifierStates(0), lastLevelCode(0), state(KeyLevel0)  {}
	int getModifierIndex(Key modifier) {
		switch(modifier) {
			case Key::Left_Control: return 1;
			case Key::Right_Control: return 2;
			case Key::Left_Alt: return 3;
			case Key::Right_Alt: return 4;
			case Key::Numberlock: return 5;
			case Key::Left_Shift: return 6;
			case Key::Right_Shift: return 7;
			case Key::Capslock: return 8;
			default: return 0;
		}
	}
	void modifierPressed(Key modifier) {
		int idx = getModifierIndex(modifier);
		if(idx == 0) return;
		modifierStates |= ((u64)1 << idx);
	}
	void modifierReleased(Key modifier) {
		int idx = getModifierIndex(modifier);
		modifierStates &= ~((u64)1 << idx);
	}
	bool isModifierPressed(Key modifier) {
		int idx = getModifierIndex(modifier);
		return (modifierStates & ((u64)1 << idx)) != 0;
	}
	bool isControlPressed() {
		return isModifierPressed(Key::Left_Control) || isModifierPressed(Key::Right_Control);
	}
	bool isAltPressed() {
		return isModifierPressed(Key::Left_Alt) || isModifierPressed(Key::Right_Alt);
	}
	bool isNumberlockPressed() {
		return isModifierPressed(Key::Numberlock);
	}
	bool isShiftPressed() {
		return isModifierPressed(Key::Left_Shift) || isModifierPressed(Key::Right_Shift);
	}
	bool isCapslockPressed() {
		return isModifierPressed(Key::Capslock);
	}
	bool handleKeyLevel0(u8 byte) {
		switch(byte) {
			case 0x01: {
				keys.put(Key::F9);
				return true;
			}
			case 0x03: {
				keys.put(Key::F5);
				return true;
			}
			case 0x04: {
				keys.put(Key::F3);
				return true;
			}
			case 0x05: {
				keys.put(Key::F1);
				return true;
			}
			case 0x06: {
				keys.put(Key::F2);
				return true;
			}
			case 0x07: {
				keys.put(Key::F12);
				return true;
			}
			case 0x09: {
				keys.put(Key::F10);
				return true;
			}
			case 0x0A: {
				keys.put(Key::F8);
				return true;
			}
			case 0x0B: {
				keys.put(Key::F6);
				return true;
			}
			case 0x0C: {
				keys.put(Key::F4);
				return true;
			}
			case 0x0D: {
				keys.put(Key::Tab);
				return true;
			}
			case 0x0E: {
				keys.put(Key::Sym_Backtick);
				return true;
			}
			case 0x11: {
				keys.put(Key::Left_Alt);
				modifierPressed(Key::Left_Alt);
				return true;
			}
			case 0x12: {
				keys.put(Key::Left_Shift);
				modifierPressed(Key::Left_Shift);
				return true;
			}
			case 0x14: {
				keys.put(Key::Left_Control);
				modifierPressed(Key::Left_Control);
				return true;
			}
			case 0x15: {
				keys.put(Key::Alpha_Q);
				return true;
			}
			case 0x16: {
				keys.put(Key::Num_1);
				return true;
			}
			case 0x1A: {
				keys.put(Key::Alpha_Z);
				return true;
			}
			case 0x1B: {
				keys.put(Key::Alpha_S);
				return true;
			}
			case 0x1C: {
				keys.put(Key::Alpha_A);
				return true;
			}
			case 0x1D: {
				keys.put(Key::Alpha_W);
				return true;
			}
			case 0x1E: {
				keys.put(Key::Num_2);
				return true;
			}
			case 0x21: {
				keys.put(Key::Alpha_C);
				return true;
			}
			case 0x22: {
				keys.put(Key::Alpha_X);
				return true;
			}
			case 0x23: {
				keys.put(Key::Alpha_D);
				return true;
			}
			case 0x24: {
				keys.put(Key::Alpha_E);
				return true;
			}
			case 0x25: {
				keys.put(Key::Num_4);
				return true;
			}
			case 0x26: {
				keys.put(Key::Num_3);
				return true;
			}
			case 0x29: {
				keys.put(Key::Space);
				return true;
			}
			case 0x2A: {
				keys.put(Key::Alpha_V);
				return true;
			}
			case 0x2B: {
				keys.put(Key::Alpha_F);
				return true;
			}
			case 0x2C: {
				keys.put(Key::Alpha_T);
				return true;
			}
			case 0x2D: {
				keys.put(Key::Alpha_R);
				return true;
			}
			case 0x2E: {
				keys.put(Key::Num_5);
				return true;
			}
			case 0x31: {
				keys.put(Key::Alpha_N);
				return true;
			}
			case 0x32: {
				keys.put(Key::Alpha_B);
				return true;
			}
			case 0x33: {
				keys.put(Key::Alpha_H);
				return true;
			}
			case 0x34: {
				keys.put(Key::Alpha_G);
				return true;
			}
			case 0x35: {
				keys.put(Key::Alpha_Y);
				return true;
			}
			case 0x36: {
				keys.put(Key::Num_6);
				return true;
			}
			case 0x3A: {
				keys.put(Key::Alpha_M);
				return true;
			}
			case 0x3B: {
				keys.put(Key::Alpha_J);
				return true;
			}
			case 0x3C: {
				keys.put(Key::Alpha_U);
				return true;
			}
			case 0x3D: {
				keys.put(Key::Num_7);
				return true;
			}
			case 0x3E: {
				keys.put(Key::Num_8);
				return true;
			}
			case 0x41: {
				keys.put(Key::Sym_Comma);
				return true;
			}
			case 0x42: {
				keys.put(Key::Alpha_K);
				return true;
			}
			case 0x43: {
				keys.put(Key::Alpha_I);
				return true;
			}
			case 0x44: {
				keys.put(Key::Alpha_O);
				return true;
			}
			case 0x45: {
				keys.put(Key::Num_0);
				return true;
			}
			case 0x46: {
				keys.put(Key::Num_9);
				return true;
			}
			case 0x49: {
				keys.put(Key::Sym_Dot);
				return true;
			}
			case 0x4A: {
				keys.put(Key::Sym_Backslash);
				return true;
			}
			case 0x4B: {
				keys.put(Key::Alpha_L);
				return true;
			}
			case 0x4C: {
				keys.put(Key::Sym_Semicolon);
				return true;
			}
			case 0x4D: {
				keys.put(Key::Alpha_P);
				return true;
			}
			case 0x4E: {
				keys.put(Key::Sym_Minus);
				return true;
			}
			case 0x52: {
				keys.put(Key::Sym_Singlequote);
				return true;
			}
			case 0x54: {
				keys.put(Key::Sym_Squareopen);
				return true;
			}
			case 0x55: {
				keys.put(Key::Sym_Equals);
				return true;
			}
			case 0x58: {
				keys.put(Key::Capslock);
				modifierPressed(Key::Capslock);
				return true;
			}
			case 0x59: {
				keys.put(Key::Right_Shift);
				modifierPressed(Key::Right_Shift);
				return true;
			}
			case 0x5A: {
				keys.put(Key::Enter);
				return true;
			}
			case 0x5B: {
				keys.put(Key::Sym_Squareclose);
				return true;
			}
			case 0x5D: {
				keys.put(Key::Sym_Forwardslash);
				return true;
			}
			case 0x66: {
				keys.put(Key::Backspace);
				return true;
			}
			case 0x69: {
				keys.put(Key::Num_Keypad_1);
				return true;
			}
			case 0x6B: {
				keys.put(Key::Num_Keypad_4);
				return true;
			}
			case 0x6C: {
				keys.put(Key::Num_Keypad_7);
				return true;
			}
			case 0x70: {
				keys.put(Key::Num_Keypad_0);
				return true;
			}
			case 0x71: {
				keys.put(Key::Sym_Keypad_Dot);
				return true;
			}
			case 0x72: {
				keys.put(Key::Num_Keypad_2);
				return true;
			}
			case 0x73: {
				keys.put(Key::Num_Keypad_5);
				return true;
			}
			case 0x74: {
				keys.put(Key::Num_Keypad_6);
				return true;
			}
			case 0x75: {
				keys.put(Key::Num_Keypad_8);
				return true;
			}
			case 0x76: {
				keys.put(Key::Escape);
				return true;
			}
			case 0x77: {
				keys.put(Key::Numberlock);
				modifierPressed(Key::Numberlock);
				return true;
			}
			case 0x78: {
				keys.put(Key::F11);
				return true;
			}
			case 0x79: {
				keys.put(Key::Sym_Keypad_Plus);
				return true;
			}
			case 0x7A: {
				keys.put(Key::Num_Keypad_3);
				return true;
			}
			case 0x7B: {
				keys.put(Key::Sym_Keypad_Minus);
				return true;
			}
			case 0x7C: {
				keys.put(Key::Sym_Keypad_Star);
				return true;
			}
			case 0x7D: {
				keys.put(Key::Num_Keypad_9);
				return true;
			}
			case 0x7E: {
				keys.put(Key::Scrolllock);
				return true;
			}
			case 0x83: {
				keys.put(Key::F7);
				return true;
			}
			case 0xE0: {
				lastLevelCode = 0xE0;
				state = KeyLevel1;
				return false;
			}
			case 0xF0: {
				lastLevelCode = 0xF0;
				state = KeyLevel1;
				return false;
			}
			case 0xE1: {
				lastLevelCode = 0xE1;
				state = KeyLevel1;
				return false;
			}
			default: {
				state = KeyLevel0;
				return false;
			}
		}
		return false;
	}
	

	bool handleKeyLevel1(u8 byte) {
		if(lastLevelCode == 0xE0) {
			switch(byte) {
				case 0x10: {
					keys.put(Key::Multimedia_Www_Search);
					return true;
				}
				case 0x11: {
					keys.put(Key::Right_Alt);
					modifierPressed(Key::Right_Alt);
					return true;
				}
				case 0x14: {
					keys.put(Key::Right_Control);
					modifierPressed(Key::Right_Control);
					return true;
				}
				case 0x15: {
					keys.put(Key::Multimedia_Previous_Track);
					return true;
				}
				case 0x18: {
					keys.put(Key::Multimedia_Www_Favourites);
					return true;
				}
				case 0x1F: {
					keys.put(Key::Left_Gui);
					return true;
				}
				case 0x20: {
					keys.put(Key::Multimedia_Www_Refresh);
					return true;
				}
				case 0x21: {
					keys.put(Key::Multimedia_Volume_Down);
					return true;
				}
				case 0x23: {
					keys.put(Key::Multimedia_Mute);
					return true;
				}
				case 0x27: {
					keys.put(Key::Right_Gui);
					return true;
				}
				case 0x28: {
					keys.put(Key::Multimedia_Www_Stop);
					return true;
				}
				case 0x2B: {
					keys.put(Key::Multimedia_Calculator);
					return true;
				}
				case 0x2F: {
					keys.put(Key::Apps);
					return true;
				}
				case 0x30: {
					keys.put(Key::Multimedia_Www_Forward);
					return true;
				}
				case 0x32: {
					keys.put(Key::Multimedia_Volume_Up);
					return true;
				}
				case 0x34: {
					keys.put(Key::Multimedia_Play_Pause);
					return true;
				}
				case 0x37: {
					keys.put(Key::Acpi_Power);
					return true;
				}
				case 0x38: {
					keys.put(Key::Multimedia_Www_Back);
					return true;
				}
				case 0x3A: {
					keys.put(Key::Multimedia_Www_Home);
					return true;
				}
				case 0x3B: {
					keys.put(Key::Multimedia_Stop);
					return true;
				}
				case 0x3F: {
					keys.put(Key::Acpi_Sleep);
					return true;
				}
				case 0x40: {
					keys.put(Key::Multimedia_My_Computer);
					return true;
				}
				case 0x48: {
					keys.put(Key::Multimedia_Email);
					return true;
				}
				case 0x4A: {
					keys.put(Key::Sym_Keypad_Backslash);
					return true;
				}
				case 0x4D: {
					keys.put(Key::Multimedia_Next_Track);
					return true;
				}
				case 0x50: {
					keys.put(Key::Multimedia_Media_Select);
					return true;
				}
				case 0x5A: {
					keys.put(Key::Keypad_Enter);
					return true;
				}
				case 0x5E: {
					keys.put(Key::Acpi_Wake);
					return true;
				}
				case 0x69: {
					keys.put(Key::End);
					return true;
				}
				case 0x6B: {
					keys.put(Key::Cursor_Left);
					return true;
				}
				case 0x6C: {
					keys.put(Key::Home);
					return true;
				}
				case 0x70: {
					keys.put(Key::Insert);
					return true;
				}
				case 0x71: {
					keys.put(Key::Delete);
					return true;
				}
				case 0x72: {
					keys.put(Key::Cursor_Down);
					return true;
				}
				case 0x74: {
					keys.put(Key::Cursor_Right);
					return true;
				}
				case 0x75: {
					keys.put(Key::Cursor_Up);
					return true;
				}
				case 0x7A: {
					keys.put(Key::Page_Down);
					return true;
				}
				case 0x7D: {
					keys.put(Key::Page_Up);
					return true;
				}
				case 0x12: {
					lastLevelCode = 0x12;
					state = KeyLevel2;
					return false;
				}
				case 0xF0: {
					lastLevelCode = 0xF0;
					state = KeyLevel2;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0xF0) {
			switch(byte) {
				case 0x11: {
					// Left_Alt released
					modifierReleased(Key::Left_Alt);
					return true;
				}
				case 0x12: {
					// Left_Shift released
					modifierReleased(Key::Left_Shift);
					return true;
				}
				case 0x14: {
					// Left_Control released
					modifierReleased(Key::Left_Control);
					return true;
				}
				case 0x58: {
					// Capslock released
					modifierReleased(Key::Capslock);
					return true;
				}
				case 0x59: {
					// Right_Shift released
					modifierReleased(Key::Right_Shift);
					return true;
				}
				case 0x77: {
					// Numberlock released
					modifierReleased(Key::Numberlock);
					return true;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0xE1) {
			switch(byte) {
				case 0x14: {
					lastLevelCode = 0x14;
					state = KeyLevel2;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		return false;
	}
	

	bool handleKeyLevel2(u8 byte) {
		if(lastLevelCode == 0x12) {
			switch(byte) {
				case 0xE0: {
					lastLevelCode = 0xE0;
					state = KeyLevel3;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0xF0) {
			switch(byte) {
				case 0x11: {
					// Right_Alt released
					modifierReleased(Key::Right_Alt);
					return true;
				}
				case 0x14: {
					// Right_Control released
					modifierReleased(Key::Right_Control);
					return true;
				}
				case 0x7C: {
					lastLevelCode = 0x7C;
					state = KeyLevel3;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0x14) {
			switch(byte) {
				case 0x77: {
					lastLevelCode = 0x77;
					state = KeyLevel3;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		return false;
	}
	

	bool handleKeyLevel3(u8 byte) {
		if(lastLevelCode == 0xE0) {
			switch(byte) {
				case 0x7C: {
					keys.put(Key::Print_Screen);
					return true;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0x7C) {
			switch(byte) {
				case 0xE0: {
					lastLevelCode = 0xE0;
					state = KeyLevel4;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0x77) {
			switch(byte) {
				case 0xE1: {
					lastLevelCode = 0xE1;
					state = KeyLevel4;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		return false;
	}
	

	bool handleKeyLevel4(u8 byte) {
		if(lastLevelCode == 0xE0) {
			switch(byte) {
				case 0xF0: {
					lastLevelCode = 0xF0;
					state = KeyLevel5;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0xE1) {
			switch(byte) {
				case 0xF0: {
					lastLevelCode = 0xF0;
					state = KeyLevel5;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		return false;
	}
	

	bool handleKeyLevel5(u8 byte) {
		if(lastLevelCode == 0xF0) {
			switch(byte) {
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		else if(lastLevelCode == 0xF0) {
			switch(byte) {
				case 0x14: {
					lastLevelCode = 0x14;
					state = KeyLevel6;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		return false;
	}
	

	bool handleKeyLevel6(u8 byte) {
		if(lastLevelCode == 0x14) {
			switch(byte) {
				case 0xF0: {
					lastLevelCode = 0xF0;
					state = KeyLevel7;
					return false;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		return false;
	}
	

	bool handleKeyLevel7(u8 byte) {
		if(lastLevelCode == 0xF0) {
			switch(byte) {
				case 0x77: {
					keys.put(Key::Pause);
					return true;
				}
				default: {
					state = KeyLevel0;
					return false;
				}
			}
		}
		return false;
	}
	

	char getNextASCII() {
		if(keys.size() == 0) return 0;
		Key k = keys.get();
		char keyMapping[] = {
			0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0, '\b', 0, 0, 0, 
			0, 0, 0, 0, '\n', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\n', 0, 0, 0, 
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '0', '1', '2', '3', 
			'4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8', 
			'9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ' ', '/', '`', ',', '.', '=', '\\', '/', 
			'.', '-', '+', '*', '-', ';', '\'', ']', '[', '\t', };
		char res = keyMapping[k];
		if(isCapslockPressed() && res >= 'a' && res <= 'z' && !isShiftPressed()) {
			res = res - 32;
		}
		if(res != 0 && isShiftPressed()) {
			char shiftTable[] = {
				0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 
				'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ')', '!', '@', '#', 
				'$', '%', '^', '&', '*', '(', ')', '!', '@', '#', '$', '%', '^', '&', '*', 
				'(', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '?', '`', '<', '>', '+', '|', '?', '>', 
				'_', 0, 0, '_', ':', 0, 0, 0, 0, };
			res = shiftTable[k];
		}
		return res;
	}
	enum KeyLevel {
		KeyLevel0, KeyLevel1, KeyLevel2, KeyLevel3, KeyLevel4, KeyLevel5, KeyLevel6, 
		KeyLevel7, };
	KeyLevel state;
	bool handleKey(u8 key) {
		switch(state) {
			case KeyLevel0: return handleKeyLevel0(key);
			case KeyLevel1: return handleKeyLevel1(key);
			case KeyLevel2: return handleKeyLevel2(key);
			case KeyLevel3: return handleKeyLevel3(key);
			case KeyLevel4: return handleKeyLevel4(key);
			case KeyLevel5: return handleKeyLevel5(key);
			case KeyLevel6: return handleKeyLevel6(key);
			case KeyLevel7: return handleKeyLevel7(key);
		}
		return false;
	}
};
