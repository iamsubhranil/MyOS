#include <arch/x86/kernel_layout.h>
#include <boot/multiboot.h>
#include <drivers/terminal.h>

void Multiboot::dump() const {
	Terminal::write("Flags: ", Terminal::Mode::HexOnce, flags, " ( ");
	const char *f[] = {"Mem",  "Device", "Cmdline", "Mod",    "Aout", "Elf",
	                   "Mmap", "Drive",  "Config",  "Loader", "Apm",  "Vbe"};
	for(u32 start = 0x001, i = 0; start <= 0x800; i++, start <<= 1) {
		if(flags & start) {
			Terminal::write(f[i], " ");
		}
	}
	Terminal::write(")\n");
#define WRITE(x) Terminal::write(#x, ": ", x, "\n");
#define WRITEHEX(x) Terminal::write(Terminal::Mode::HexOnce, #x, ": ", x, "\n");
	WRITEHEX(mem_lower);
	WRITEHEX(mem_upper);
	WRITEHEX(boot_device);
	Terminal::write("cmdline: ", Terminal::Mode::HexOnce, cmdline, " ( ",
	                (const char *)P2V(cmdline), ")\n");

	WRITE(mods_count);
	WRITEHEX(mods_addr);

	if(flags & (1 << 5)) {
		WRITE(num);
		WRITE(size);
		WRITE(addr);
		WRITE(shndx);

		Elf32::Header *headers     = (Elf32::Header *)P2V(addr);
		char          *strtab      = NULL;
		u32            strtab_size = 0;
		Elf32::Symtab *symtab      = NULL;
		u32            symtab_size = 0;
		for(u32 i = 0; i < num; i++) {
			Elf32::Header h = headers[i];
			if(h.type == Elf32::Header::Type::Strtab) {
				strtab      = (char *)P2V(h.addr);
				strtab_size = h.size;
			} else if(h.type == Elf32::Header::Type::Symtab) {
				symtab      = (Elf32::Symtab *)P2V(h.addr);
				symtab_size = h.size;
			}
		}

		if(symtab == NULL || strtab == NULL) {
			Terminal::warn("ELF symbols not loaded! Stacktrace will not "
			               "contain symbol names!");
		} else {
			Terminal::write(
			    "Symbol and string tables loaded! Symtab: ", symtab_size,
			    " bytes, Strtab: ", strtab_size, " bytes\n");
		}
	}

	WRITE(mmap_length);
	WRITEHEX(mmap_addr);

	u8 nummaps = mmap_length / sizeof(MemoryMap);
	Terminal::write("    Base    \t    Length    \t    Type\n");
	uptr mmap_ptr = (uptr)mmap_addr;
	for(u8 i = 0; i < nummaps; i++, mmap_ptr += sizeof(MemoryMap)) {
		MemoryMap *m = (MemoryMap *)P2V(mmap_ptr);
		Terminal::write(Terminal::Mode::HexOnce, m->base_addr, "\t");
		u64 l = m->length;
		if(l > 1024) {
			l = l / 1024;
			if(l > 1024) {
				l = l / 1024;
				if(l > 1024) {
					l = l / 1024;
					Terminal::write(l, " GiB");
					goto print_mem_type;
				}
				Terminal::write(l, " MiB");
				goto print_mem_type;
			}
			Terminal::write(l, " KiB");
			goto print_mem_type;
		}
		Terminal::write(l, " B");
	print_mem_type:
		Terminal::write("\t");
		switch(m->type) {
#define MEMTYPE(x) \
	case MemoryMap::Type::x: Terminal::write(#x); break;
			MEMTYPE(Usable);
			MEMTYPE(Reserved);
			MEMTYPE(AcpiReclaim);
			MEMTYPE(AcpiNvs);
			MEMTYPE(BadMem);
		}
		Terminal::write("\n");
	}

	// WRITE(drives_length);
	// WRITEHEX(drives_addr);
	// WRITE(config_table);
	Terminal::write("boot_loader_name: ", Terminal::Mode::HexOnce,
	                boot_loader_name, " ( ",
	                (const char *)P2V(boot_loader_name), " )\n");
	// WRITE(apm_table);
	/*
	WRITE(vbe_control_info);
	WRITE(vbe_mode_info);
	WRITE(vbe_mode);
	WRITE(vbe_interface_seg);
	WRITE(vbe_interface_off);
	WRITE(vbe_interface_len);
	WRITEHEX(frameBuffer.addr);
	WRITE(frameBuffer.pitch);
	WRITE(frameBuffer.width);
	WRITE(frameBuffer.height);
	WRITE((u16)frameBuffer.bpp);
	WRITE((u16)frameBuffer.type);
	*/
}

const char *Multiboot::Elf32::Header::getName(const Multiboot *boot) {
	Header *shStrTab = boot->shndx + (Header *)(uptr)boot->addr;
	return (char *)(uptr)shStrTab->addr + name;
}
