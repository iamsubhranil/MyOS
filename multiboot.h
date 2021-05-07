#pragma once

#include "myos.h"

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
	u32 framebuffer_addr;
	u32 framebuffer_pitch;
	u32 framebuffer_width;
	u32 framebuffer_height;
	u8  framebuffer_bpp;
	u8  framebuffer_type;
	/* Palette stuff goes here but we don't use it */

	void dump() const; // dump to terminal

	struct Module {
		u32 mod_start;
		u32 mod_end;
		u32 cmdline;
		u32 reserved;
	} __attribute__((packed));

	struct MemoryMap {
		u32 size;
		u64 base_addr;
		u64 length;
		u32 type;
	} __attribute__((packed));

	struct VbeInfo {
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
		u32 reserved1;
		u16 reserved2;
	} __attribute__((packed));

} __attribute__((packed));
