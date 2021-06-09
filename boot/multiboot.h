#pragma once

#include <sys/myos.h>

struct Multiboot {

	static constexpr u32 Magic    = 0x1BADB002;
	static constexpr u32 EaxMagic = 0X2BADB002;

	enum Flag : u16 {
		Mem     = 0x001,
		Device  = 0x002,
		Cmdline = 0x004,
		Mods    = 0x008,
		Aout    = 0x010,
		Elf     = 0x020,
		Mmap    = 0x040,
		Drive   = 0x080,
		Config  = 0x100,
		Loader  = 0x200,
		Apm     = 0x400,
		Vbe     = 0x800
	};

	u32 flags;
	u32 mem_lower;
	u32 mem_upper;
	u32 boot_device;
	u32 cmdline;
	u32 mods_count;
	u32 mods_addr;
	u32 num;
	u32 size;
	u32 addr;
	u32 shndx;
	u32 mmap_length;
	u32 mmap_addr;
	u32 drives_length;
	u32 drives_addr;
	u32 config_table;
	u32 boot_loader_name;
	u32 apm_table;
	u32 vbe_control_info;
	u32 vbe_mode_info;
	u32 vbe_mode;
	u32 vbe_interface_seg;
	u32 vbe_interface_off;
	u32 vbe_interface_len;

	void dump() const; // dump to terminal

	struct Framebuffer {
		enum Type : u8 {
			Indexed = 0,
			RGB     = 1,
			EGAText = 2,
		};

		u32  addr;
		u32  pitch;
		u32  width;
		u32  height;
		u8   bpp;
		Type type;

		union {
			struct {
				u32 palette_addr;
				u16 palette_num_colors;
			};
			struct {
				u8 red_field_position;
				u8 red_mask_size;
				u8 green_field_position;
				u8 green_mask_size;
				u8 blue_field_position;
				u8 blue_mask_size;
			};
		};
	} __attribute__((packed));

	Framebuffer frameBuffer;

	struct Module {
		u32 mod_start;
		u32 mod_end;
		u32 cmdline;
		u32 reserved;
	} __attribute__((packed));

	struct MemoryMap {
		enum Type : u32 {
			Usable      = 1,
			Reserved    = 2,
			AcpiReclaim = 3,
			AcpiNvs     = 4,
			BadMem      = 5
		};

		u32  size;
		u64  base_addr;
		u64  length;
		Type type;
		// u32  acpi3ea;
	} __attribute__((packed));

	struct VbeControlInfo {
		char signature[4];  // must be "VESA" to indicate valid VBE support
		u16  version;       // VBE version; high byte is major version, low byte
		                    // is minor version
		u32 oem;            // segment:offset pointer to OEM
		u32 capabilities;   // bitfield that describes card capabilities
		u32 video_modes;    // segment:offset pointer to list of supported video
		                    // modes
		u16  video_memory;  // amount of video memory in 64KB blocks
		u16  software_rev;  // software revision
		u32  vendor;        // segment:offset to card vendor string
		u32  product_name;  // segment:offset to card model name
		u32  product_rev;   // segment:offset pointer to product revision
		char reserved[222]; // reserved for future expansion
		char oem_data[256]; // OEM BIOSes store their strings in this area
	} __attribute__((packed));

	struct VbeModeInfo {
		u16 attributes;
		u8  winA, winB;
		u16 granularity;
		u16 winsize;
		u16 segmentA, segmentB;
		u32 realFctPtr;
		u16 pitch;

		u16 Xres, Yres;
		u8  Wchar, Ychar, planes, bpp, banks;
		u8  memory_model, bank_size, image_pages;
		u8  reserved0;

		u8 red_mask, red_position;
		u8 green_mask, green_position;
		u8 blue_mask, blue_position;
		u8 rsv_mask, rsv_position;
		u8 directcolor_attributes;

		u32 physbase;
		u32 off_screen_mem_off;
		u16 off_screen_mem_size;
		u8  resvered[206];
	} __attribute__((packed));

} __attribute__((packed));
