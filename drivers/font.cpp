#include <drivers/font.h>
#include <drivers/fonts/cascadiamono.h>

Font Font::CurrentFont = CascadiaMono();

void Font::init() {
	CurrentFont = CascadiaMono();
}

VGA::Point Font::Renderer::render(VGA::Point p, char c, u32 color) {
	u8         h   = CurrentFont.getHeight();
	u8         w   = CurrentFont.getCharacterWidth(c);
	const u8 * g   = CurrentFont.getCharacterGlyph(c);
	VGA::Point bak = p;
	// horribly inefficient
	for(u8 i = 0; i < h; i++) {
		bak.x = p.x;
		for(u8 j = 0; j < w; j++) {
			u8  intensity = g[i * w + j];
			u32 red = (color & (0xFF << VGA::RedPosition)) >> VGA::RedPosition;
			u32 green =
			    (color & (0xFF << VGA::GreenPosition)) >> VGA::GreenPosition;
			u32 blue =
			    (color & (0xFF << VGA::BluePosition)) >> VGA::BluePosition;

			red          = red * intensity / 255;
			green        = green * intensity / 255;
			blue         = blue * intensity / 255;
			u32 newColor = (red << VGA::RedPosition) |
			               (green << VGA::GreenPosition) |
			               (blue << VGA::BluePosition);
			VGA::setPixel(bak, newColor);
			bak.x++;
		}
		bak.y++;
	}
	p.x += w;
	// VGA::flush();
	return p;
}

VGA::Point Font::Renderer::render(VGA::Point p, const char *text, u32 color) {
	VGA::Point charStart = p;
	while(*text) {
		charStart = render(charStart, *text++, color);
	}
	return charStart;
}

VGA::Point Font::Renderer::render(VGA::Point p, const char *text, siz size,
                                  u32 color) {
	VGA::Point charStart = p;
	while(size > 0) {
		charStart = render(charStart, *text++, color);
		size--;
	}
	return charStart;
}
