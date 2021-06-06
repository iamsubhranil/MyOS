#pragma once
#include <boot/multiboot.h>
#include <sys/myos.h>

struct VGA {
	// gruvbox palatte
	enum Color : u32 {
		Black        = 0x1d2021,
		Red          = 0xcc241d,
		Green        = 0x98971a,
		Yellow       = 0xd79921,
		Blue         = 0x458588,
		Magenta      = 0xb16286,
		Cyan         = 0x689d6a,
		White        = 0xebdbb2,
		LightBlack   = 0x3c3836,
		LightRed     = 0xfb4934,
		LightGreen   = 0xb8bb26,
		LightYellow  = 0xfabd2f,
		LightBlue    = 0x83a598,
		LightMagenta = 0xd3869b,
		LightCyan    = 0x8ec07c,
		LightWhite   = 0xfbf1c7,
		Orange       = 0xd65d0e,
		Gray         = 0x928374,
		LightOrange  = 0xfe8019,
		LightGrey    = 0xa89984,
	};

	static constexpr Color Colors[] = {
	    Black,      Red,         Green,     Yellow,       Blue,
	    Magenta,    Cyan,        White,     LightBlack,   LightRed,
	    LightGreen, LightYellow, LightBlue, LightMagenta, LightCyan,
	    LightWhite, Orange,      Gray,      LightOrange,  LightGrey,
	};

	struct Point {
		u16 x, y;

		Point(u32 a, u32 b) : x(a), y(b) {
		}
	};

	struct Color2 {
		u8 r, g, b;
		Color2(u8 x, u8 y, u8 z) : r(x), g(y), b(z) {
		}
		operator u32() {
			return (u32)r << VGA::RedPosition | (u32)g << VGA::GreenPosition |
			       (u32)b << VGA::BluePosition;
		}
	};

	static void init(Multiboot::VbeModeInfo *vbem);
	// assume 32 bit framebuffer for now
	static u32 *FrameBuffer;
	static u8   BitsPerPixel;
	static u16  Pitch;
	static u16  Width;
	static u16  Height;

	static u8 RedMask, RedPosition;
	static u8 GreenMask, GreenPosition;
	static u8 BlueMask, BluePosition;

	static inline u32 *pixelAt(Point p) {
		return (u32 *)(p.y * Pitch + (p.x * (BitsPerPixel / 8)) +
		               (u8 *)FrameBuffer);
	}
	static void setPixel(Point p, u32 color);
	static void drawLine(Point start, Point end, u32 color);
	static void drawLineHorizontal(Point start, u16 width, u32 color,
	                               u16 thickness = 1);
	static void drawLineVertical(Point start, u16 thickness, u16 height,
	                             u32 color);
	static void drawRect(Point topLeft, Point bottomRight, u32 outlineColor,
	                     u32 outlineWidth = 1, u32 fillColor = 0xFF000000);
	static void drawCircle(Point center, u16 radius, u32 outlineColor,
	                       u32 outlineWidth = 1, u32 fillColor = 0xFF000000);
	// brings the given point to 0,0, fills the empty space at the end with 0
	static void scrollForward(Point p);
};
