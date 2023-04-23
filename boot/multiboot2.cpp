#include <boot/multiboot2.h>
#include <drivers/terminal.h>
#include <sys/string.h>

// #ifdef MYOS_USE_MULTIBOOT2

#define MEMF(x) Terminal::write(#x ": ", (uptr)x(), "\n");
#define MEM(x) Terminal::write(#x ": ", x, "\n");
#define MEMHEX(x) \
	Terminal::write(#x ": ", Terminal::Mode::HexOnce, (uptr)x, "\n");
#define MEMFHEX(x) Terminal::write(#x ": ", Terminal::Mode::HexOnce, x(), "\n");
#define MEMSTR(x) Terminal::write(#x ": ", (const char *)((uptr)x), "\n");

void Multiboot2::Module::dump() const {
	MEMHEX(mod_start);
	MEMHEX(mod_end);
	MEMF(cmdline);
}

void Multiboot2::BasicMeminfo::dump() const {
	MEMHEX(mem_lower);
	MEMHEX(mem_upper);
}

void Multiboot2::Bootdev::dump() const {
	MEMHEX(biosdev);
	MEMHEX(slice);
	MEMHEX(part);
}

void Multiboot2::Mmap::dump() const {
	MEM(entry_size);
	MEM(entry_version);
	u32 entry_count = (tag.size - sizeof(Mmap)) / entry_size;
	Terminal::write("\tAddr\t\tLength\t\tType\n");
	Terminal::write("=================================\n");
	for(u32 i = 0; i < entry_count; i++) {
		Entry e = entries()[i];
		Terminal::write(Terminal::Mode::HexOnce, e.addr, "\t", e.len, "\t");
		switch(e.type) {
			case Entry::Type::Reserved: Terminal::write("Reserved"); break;
			case Entry::Type::Available: Terminal::write("Available"); break;
			case Entry::Type::AcpiReclaimable: Terminal::write("ACPI"); break;
			case Entry::Type::Nvs: Terminal::write("NVS"); break;
			case Entry::Type::BadRam: Terminal::write("BAD"); break;
		}
		Terminal::write("\n");
	}
}

void Multiboot2::Vbe::dump() const {
	MEMHEX(mode);
	MEMHEX(interface_seg);
	MEMHEX(interface_off);
	MEMHEX(interface_len);
	Terminal::write("\nControlInfo:\n");
	MEMHEX(control_info.signature);
	MEMHEX(control_info.version);
	MEMSTR(control_info.oemStringPtr);
	MEMHEX(control_info.capabilities);
	MEMHEX(control_info.videoModePtr);
	MEMHEX(control_info.totalMemory * 64 * 1024);
	Terminal::write("\nModeInfo:\n");
	MEMHEX(mode_info.window_size);
	MEMHEX(mode_info.segment_a);
	MEMHEX(mode_info.segment_b);
	MEM(mode_info.pitch);
	MEM(mode_info.width);
	MEM(mode_info.height);
	MEM(mode_info.bpp);
	MEMHEX(mode_info.red_mask);
	MEM(mode_info.red_position);
	MEMHEX(mode_info.green_mask);
	MEM(mode_info.green_position);
	MEMHEX(mode_info.blue_mask);
	MEM(mode_info.blue_position);
	MEMHEX(mode_info.direct_color_attributes);
	MEMHEX(mode_info.framebuffer);
	MEMHEX(mode_info.off_screen_mem_off);
	MEM(mode_info.off_screen_mem_size);
}

void Multiboot2::ElfSections::dump() const {
	MEMHEX(num);
	MEMHEX(entsize);
	MEMHEX(sections());
	MEMHEX(shndx);
}

void Multiboot2::Apm::dump() const {
	MEMHEX(version);
	MEMHEX(cseg);
	MEMHEX(offset);
	MEMHEX(cseg_16);
	MEMHEX(dseg);
	MEMHEX(flags);
	MEMHEX(cseg_len);
	MEMHEX(cseg_16_len);
	MEMHEX(dseg_len);
}

void Multiboot2::Efi32::dump() const {
	MEMHEX(pointer);
}

void Multiboot2::Efi64::dump() const {
	MEMHEX(pointer);
}

void Multiboot2::SMBios::dump() const {
	MEMHEX(major);
	MEMHEX(minor);
	MEMF(tables);
}

void Multiboot2::AcpiOld::dump() const {
	MEMF(rsdp);
}

void Multiboot2::AcpiNew::dump() const {
	MEMF(rsdp);
}

void Multiboot2::Network::dump() const {
	MEMF(dhcpack);
}

void Multiboot2::EfiMmap::dump() const {
	MEMHEX(descr_size);
	MEMHEX(descr_vers);
	MEMF(efi_mmap);
}

void Multiboot2::Efi32Ih::dump() const {
	MEMHEX(pointer);
}

void Multiboot2::Efi64Ih::dump() const {
	MEMHEX(pointer);
}

void Multiboot2::LoadBaseAddr::dump() const {
	MEMHEX(load_base_addr);
}

void Multiboot2::Framebuffer::dump() const {
	MEMHEX(addr);
	MEM(pitch);
	MEM(width);
	MEM(height);
	MEM(bpp);
	MEM((uptr)type);
}

bool Multiboot2::Tag::parse(Multiboot2 *multiboot) {
	switch(type) {
		case Type::End: Terminal::write("[Section] End\n"); return false;
		default:
			Terminal::warn("Ignoring unknown multiboot tag '", (uptr)type,
			               "'!");
			break;
		case Type::BootloaderName:
			multiboot->bootloaderName = (char *)(this + 1);
			Terminal::write("[Section] BootloaderName (",
			                multiboot->bootloaderName, ")\n");
			break;
		case Type::Cmdline:
			multiboot->cmdline = (char *)(this + 1);
			Terminal::write("[Section] Cmdline (", multiboot->cmdline, ")\n");
			break;
		case Type::Module:
			multiboot->modules[multiboot->moduleCount] =
			    (Multiboot2::Module *)this;
			Terminal::write("[Section] Module (#", multiboot->moduleCount,
			                ")\n");
			multiboot->modules[multiboot->moduleCount++]->dump();
			break;
		case Type::BasicMeminfo:
			multiboot->basicMemInfo = (Multiboot2::BasicMeminfo *)this;
			Terminal::write("[Section] BasicMeminfo\n");
			multiboot->basicMemInfo->dump();
			break;
		case Type::Bootdev:
			multiboot->bootdev = (Multiboot2::Bootdev *)this;
			Terminal::write("[Section] Bootdev\n");
			multiboot->bootdev->dump();
			break;
		case Type::Mmap:
			multiboot->mmap = (Multiboot2::Mmap *)this;
			Terminal::write("[Section] Mmap\n");
			multiboot->mmap->dump();
			break;
		case Type::Vbe:
			multiboot->vbe = (Multiboot2::Vbe *)this;
			Terminal::write("[Section] Vbe\n");
			multiboot->vbe->dump();
			break;
		case Type::Framebuffer:
			multiboot->framebuffer = (Multiboot2::Framebuffer *)this;
			Terminal::write("[Section] Framebuffer\n");
			multiboot->framebuffer->dump();
			break;
		case Type::ElfSections:
			multiboot->elfSections = (Multiboot2::ElfSections *)this;
			Terminal::write("[Section] ElfSections ", Terminal::Mode::HexOnce,
			                (uptr)this, "\n");
			multiboot->elfSections->dump();
			break;
		case Type::Apm:
			multiboot->apm = (Multiboot2::Apm *)this;
			Terminal::write("[Section] Apm\n");
			multiboot->apm->dump();
			break;
		case Type::Efi32:
			multiboot->efi32 = (Multiboot2::Efi32 *)this;
			Terminal::write("[Section] Efi32\n");
			multiboot->efi32->dump();
			break;
		case Type::Efi64:
			multiboot->efi64 = (Multiboot2::Efi64 *)this;
			Terminal::write("[Section] Efi64\n");
			multiboot->efi64->dump();
			break;
		case Type::SmBios:
			multiboot->smbios = (Multiboot2::SMBios *)this;
			Terminal::write("[Section] SmBios\n");
			multiboot->smbios->dump();
			break;
		case Type::AcpiOld:
			multiboot->acpiOld = (Multiboot2::AcpiOld *)this;
			Terminal::write("[Section] AcpiOld\n");
			multiboot->acpiOld->dump();
			break;
		case Type::AcpiNew:
			multiboot->acpiNew = (Multiboot2::AcpiNew *)this;
			Terminal::write("[Section] AcpiNew\n");
			multiboot->acpiNew->dump();
			break;
		case Type::Network:
			multiboot->network = (Multiboot2::Network *)this;
			Terminal::write("[Section] Network\n");
			multiboot->network->dump();
			break;
		case Type::EfiMmap:
			multiboot->efiMmap = (Multiboot2::EfiMmap *)this;
			Terminal::write("[Section] EfiMmap\n");
			multiboot->efiMmap->dump();
			break;
		case Type::Efi32Ih:
			multiboot->efi32ih = (Multiboot2::Efi32Ih *)this;
			Terminal::write("[Section] Efi32Ih\n");
			multiboot->efi32ih->dump();
			break;
		case Type::Efi64Ih:
			multiboot->efi64ih = (Multiboot2::Efi64Ih *)this;
			Terminal::write("[Section] Efi64Ih\n");
			multiboot->efi64ih->dump();
			break;
		case Type::LoadBaseAddr:
			multiboot->loadBaseAddr = (Multiboot2::LoadBaseAddr *)this;
			Terminal::write("[Section] LoadBaseAddr\n");
			multiboot->loadBaseAddr->dump();
			break;
	}
	uptr nextTag        = (uptr)multiboot->nextTag + size;
	uptr nextTagAligned = (nextTag + 7) & ~7;
	multiboot->nextTag  = (Tag *)(nextTagAligned);
	return true;
}

Multiboot2 Multiboot2::parse(uptr addr) {
	Multiboot2 m;
	memset(&m, 0, sizeof(Multiboot2));
	// ignore total size and reserved
	Terminal::write("Total size: ", *(uptr *)(addr), "\n");
	addr += 8;
	m.nextTag = (Tag *)addr;
	// parse all tags
	while(m.nextTag->parse(&m))
		;
	return m;
}

// #endif
