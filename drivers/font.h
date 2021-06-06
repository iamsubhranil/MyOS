#pragma once

#include <drivers/vga.h>
#include <sys/myos.h>

// A pretty basic font renderer. Upon query of a font,
// returns an u8 array, where each index contains the
// opacity of the corresponding pixel. height and width
// of the font needs to be queried first, and the
// returned array should contain height * width amount
// of pixels, sequentially.
//
// Various fonts can be implemented by defining their
// pixel values in a header, which can be generated using
// tools/generate_font_def.py.
//
// For now, only one font header file inclusion is supported,
// and must be done at compile time. This will change in
// the future, moving the font switching to the runtime.
//
// Each character will have different widths, but all of
// them will have a normalized height.

struct Font {

	const u8 *const *asciiGlyphs;
	const u8 *       asciiWidths;
	u8               asciiHeight;
	u8               maxWidth;
	u8               fontSize;
	const char *     fontName;

	Font(const u8 *const *g, const u8 *w, u8 h, u8 mw, u8 s, const char *n)
	    : asciiGlyphs(g), asciiWidths(w), asciiHeight(h), maxWidth(mw),
	      fontSize(s), fontName(n) {
	}

	static Font CurrentFont;

	const char *getFontName() {
		return fontName;
	}
	u8 getFontSize() {
		return fontSize;
	}
	u8 getHeight() {
		return asciiHeight;
	}
	const u8 *getCharacterGlyph(char c) {
		return asciiGlyphs[(u8)c];
	}
	u8 getCharacterWidth(char c) {
		return asciiWidths[(u8)c];
	}
	static void setFont(Font f) {
		CurrentFont = f;
	}
	static u8 charactersPerLine() {
		return VGA::Width / CurrentFont.maxWidth;
	}
	static u8 linesPerFrame() {
		return VGA::Height / CurrentFont.asciiHeight;
	}
	static void init();

	struct Renderer {
		// returns the position of the next character,
		// does not handle \n, \t, \r, does not handle wrapping
		static VGA::Point render(VGA::Point p, char c, u32 color);
		static VGA::Point render(VGA::Point p, const char *text, u32 color);
		static VGA::Point render(VGA::Point p, const char *text, siz size,
		                         u32 color);
	};
};
