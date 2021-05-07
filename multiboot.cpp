#include "multiboot.h"
#include "terminal.h"

void Multiboot::dump() const {
	Terminal::write("Flags: ", Terminal::Mode::HexOnce, flags, " (");
	const char *f[] = {"Mem",  "Device", "Cmdline", "Mod",    "Aout", "Elf",
	                   "Mmap", "Drive",  "Config",  "Loader", "Apm",  "Vbe"};
	for(u32 start = 0x001, i = 0; start <= 0x800; i++, start <<= 1) {
		if(flags & start) {
			Terminal::write(f[i]);
		}
	}
	Terminal::write(")\n");
#define WRITE(x) Terminal::write(#x, ": ", x, "\n");
#define WRITEHEX(x) Terminal::write(Terminal::Mode::HexOnce, #x, ": ", x, "\n");
	WRITEHEX(mem_lower);
	WRITEHEX(mem_upper);
	WRITE(boot_device);
	WRITE(cmdline);
	WRITE(mods_count);
	WRITEHEX(mods_addr);
	WRITE(num);
	WRITE(size);
	WRITE(addr);
	WRITE(shndx);
	WRITE(mmap_length);
	WRITEHEX(mmap_addr);
	WRITE(drives_length);
	WRITEHEX(drives_addr);
	WRITE(config_table);
	WRITEHEX(boot_loader_name);
	WRITE(apm_table);
	WRITE(vbe_control_info);
	WRITE(vbe_mode_info);
	WRITE(vbe_mode);
	WRITE(vbe_interface_seg);
	WRITE(vbe_interface_off);
	WRITE(vbe_interface_len);
	WRITEHEX(framebuffer_addr);
	WRITE(framebuffer_pitch);
	WRITE(framebuffer_width);
	WRITE(framebuffer_height);
	WRITE((u16)framebuffer_bpp);
	WRITE((u16)framebuffer_type);
}
