#pragma once
#include "myos.h"

struct VGA {
	/* Hardware text mode color constants. */
	enum Color : u8 {
		Black        = 0,
		Blue         = 1,
		Green        = 2,
		Cyan         = 3,
		Red          = 4,
		Magenta      = 5,
		Brown        = 6,
		LightGrey    = 7,
		DarkGrey     = 8,
		LightBlue    = 9,
		LightGreen   = 10,
		LightCyan    = 11,
		LightRed     = 12,
		LightMagenta = 13,
		LightBrown   = 14,
		White        = 15,
		Reset        = 16, // not a valid color, a control value for terminal
	};

	static inline Color color(Color fg, Color bg) {
		return (Color)((u8)fg | (u8)bg << 4);
	}

	static inline u16 entry(char uc, Color color) {
		return (u16)uc | (u16)color << 8;
	}

	static const size_t Width  = 80;
	static const size_t Height = 25;
};
