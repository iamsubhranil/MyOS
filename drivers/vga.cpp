#include <drivers/vga.h>
#include <mem/memory.h>
#include <sys/string.h>

u32 *VGA::PhysicalFrameBuffer = NULL;
u32 *VGA::BackBuffer          = NULL;
u8   VGA::BitsPerPixel        = 0;
u16  VGA::Pitch               = 0;
u16  VGA::Width               = 0;
u16  VGA::Height              = 0;
u8   VGA::BlueMask            = 0;
u8   VGA::BluePosition        = 0;
u8   VGA::RedMask             = 0;
u8   VGA::RedPosition         = 0;
u8   VGA::GreenMask           = 0;
u8   VGA::GreenPosition       = 0;

void VGA::init(Multiboot::VbeModeInfo *vbem) {
	PhysicalFrameBuffer = (u32 *)(uptr)vbem->physbase;
	BitsPerPixel        = vbem->bpp;
	Pitch               = vbem->pitch;
	Width               = vbem->Xres;
	Height              = vbem->Yres;
	BlueMask            = vbem->blue_mask;
	BluePosition        = vbem->blue_position;
	RedMask             = vbem->red_mask;
	RedPosition         = vbem->red_position;
	GreenMask           = vbem->green_mask;
	GreenPosition       = vbem->green_position;

	// BackBuffer = (u32 *)Memory::alloc(Pitch * Height);
	// memset(BackBuffer, 0, Pitch * Height);
	// flush();
	BackBuffer = PhysicalFrameBuffer;
}

void VGA::setPixel(Point p, u32 color) {
	*pixelAt(p) = color;
}

void VGA::drawLineHorizontal(Point p, u16 width, u32 color, u16 thickness) {
	while(thickness > 0) {
		u32 *pixel = pixelAt(p);
		u32 *end   = pixel + width;
		while(pixel < end) *pixel++ = color;
		p.y++;
		thickness--;
	}
	// flush();
}

void VGA::scrollForward(Point p) {
	u32 *start = pixelAt(p);
	u32  empty = (u8 *)start - (u8 *)BackBuffer;
	u32  rem   = Pitch * Height - empty;
	memcpy(BackBuffer, start, rem);
	memset((char *)BackBuffer + rem, 0, empty);
	// flush();
}

void VGA::flush() {
	// memcpy(PhysicalFrameBuffer, BackBuffer, Pitch * Height);
}
