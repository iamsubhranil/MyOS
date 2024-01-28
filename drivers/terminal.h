#pragma once

#include <drivers/vga.h>
#include <sched/spinlock.h>
#include <sys/myos.h>

#define PROMPT_INIT(name, color)            \
	const char     *__prompt_header = name; \
	Terminal::Color __prompt_color  = Terminal::Color::color;
#define PROMPT(...) \
	Terminal::prompt(__prompt_color, __prompt_header, __VA_ARGS__);

struct Terminal {

	enum class Output { Serial, VGA };

	enum Color {
		Black = 0,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan,
		White,
		LightBlack,
		LightRed,
		LightGreen,
		LightYellow,
		LightBlue,
		LightMagenta,
		LightCyan,
		LightWhite,
		// these are not supported by ascii
		Orange,
		Gray,
		LightOrange,
		LightGrey,
		Reset,
	};

	enum class Mode {
		Dec,
		Hex,
		Bin,
		// automatically resets the mode after printing one number
		DecOnce,
		HexOnce,
		BinOnce,
		Reset // sets back whatever the previous mode was
	};
	enum class Move {
		Up,
		Down,
		Left,
		Right
	}; // does not overwrite, just moves the cursor
	enum class Control { ClearLine };

	static Output CurrentOutput;
	static bool   SerialInited;
	static bool   VGAInited;
	static Mode   currentMode;
	static Mode   previousMode;

	// only foreground colors
	static Color color;
	static Color previousColor;

	static u16  row;
	static u16  column;
	static u16 *buffer;
	static u16  vgaLineWidth;  // number of characters per line in a row
	static u16  vgaLineHeight; // number of rows in a screen

	static SpinLock spinlock;

	// initSerial and initVga do not acquire the lock
	static void initSerial();
	static void initVga(Multiboot *m);
	static void init(Multiboot *m);

	static void setColor(Color color);
	static void setMode(Mode m);
	static void putEntryAt(u8 c, Color color, u16 x, u16 y);
	static void putEntryAt(u16 entry, u16 x, u16 y);
	static void moveUpOneRow();

	static u32 write_hex(const u64 &value);
	static u32 write_dec(const u64 &value);
	static u32 write_bin(const u64 &value);

	static u32 writebytes(const char *const &data, siz len);
	static u32 write(const char *const &data);
	static u32 write(const char &c);
	static u32 write_nolock(
	    const char &c); // writes a single character without using any lock
	static u32 write(const u64 &value);
	static u32 write(const u32 &value) {
		return write((u64)value);
	}
	static u32 write(const u16 &value) {
		return write((u64)value);
	}
	static u32 write(const u8 &value) {
		return write((u64)value);
	}
	static u32 write(const int value) {
		return write((i64)value);
	}
	static u32 write(const i64 &value);
	static u32 write(const i32 &value) {
		return write((i64)value);
	}
	static u32 write(const i16 &value) {
		return write((i64)value);
	}
	static u32 write(void *const &value) {
		return write(Mode::Hex, (uptr)value, Mode::Reset);
	}
	static u32 write(const bool &value) {
		if(value)
			return write("true");
		return write("false");
	}
	static u32 write(Mode m);
	static u32 write(Move m);
	static u32 write(Control c);
	static u32 write(Color c);
	static u32 write(Output o);

	// color conversion
	static void       writeSerialColor(Color c);
	static VGA::Color toVGAColor(Color c);

	template <size_t N> u32 write(const char (&val)[N]) {
		return writebytes(val, N);
	}
	template <typename T> static u32 write(const T &val) {
		return val.dump();
	}
	// this is needed to enforce correct ordering of the writes.
	// ordering of function call pack expansion is not
	// defined otherwise. although gcc and clang does expand
	// in order, msvc does not, unless explictly forced using this.
	// passing write(x) to the helper explictly instantiates the
	// writes in order.
	template <typename F, typename... T>
	static u32 write_helper(u32 s, const F &arg, const T &...args) {
		return s + write_helper(write(arg), args...);
	}

	static u32 write_helper(u32 s) {
		return s;
	}

	template <typename F, typename... T>
	static u32 write(const F &arg, const T &...args) {
		return write_helper(0, arg, args...);
	}

	template <typename T>
	static void prompt_internal(Color foreground, const T &prompt) {
		write("[ ", foreground, prompt, Color::Reset, " ] ");
	}

	template <typename S, typename... T>
	static void prompt(Color foreground, const S &prompt, const T &...args) {
		prompt_internal(foreground, prompt);
		write(args..., '\n');
	}

	template <typename... T> static void info(const T &...args) {
		prompt(Color::Cyan, "Info", args...);
	}

	template <typename... T> static void warn(const T &...args) {
		prompt(Color::Magenta, "Warn", args...);
	}

	template <typename... T> static void done(const T &...args) {
		prompt(Color::Green, "Done", args...);
	}

	template <typename... T> static void fail(const T &...args) {
		prompt(Color::Red, "Fail", args...);
	}

	template <typename... T> static void err(const T &...args) {
		prompt(Color::Red, "ERR ", args...);
	}
};
