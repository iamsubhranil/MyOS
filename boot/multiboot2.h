/*  multiboot2.h - Multiboot 2 header file.  */
/*  Copyright (C) 1999,2003,2007,2008,2009,2010  Free Software Foundation, Inc.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL ANY
 *  DEVELOPER OR DISTRIBUTOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <sys/myos.h>

#ifdef MYOS_USE_MULTIBOOT2

/* How many bytes from the start of the file we search for the header.  */
#define MULTIBOOT_SEARCH 32768
#define MULTIBOOT_HEADER_ALIGN 8

struct Multiboot2 {

	/* The magic field should contain this.  */
	static const u32 Magic = 0xe85250d6;

/* This should be in %eax.  */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

/* Alignment of multiboot modules.  */
#define MULTIBOOT_MOD_ALIGN 0x00001000

/* Alignment of the multiboot info structure.  */
#define MULTIBOOT_INFO_ALIGN 0x00000008

	/* Flags set in the 'flags' member of the multiboot header.  */

#define MULTIBOOT_TAG_ALIGN 8

	struct Color {
		u8 red;
		u8 green;
		u8 blue;
	};

	struct Tag {

		enum Type : u32 {
			End            = 0,
			Cmdline        = 1,
			BootloaderName = 2,
			Module         = 3,
			BasicMeminfo   = 4,
			Bootdev        = 5,
			Mmap           = 6,
			Vbe            = 7,
			Framebuffer    = 8,
			ElfSections    = 9,
			Apm            = 10,
			Efi32          = 11,
			Efi64          = 12,
			SmBios         = 13,
			AcpiOld        = 14,
			AcpiNew        = 15,
			Network        = 16,
			EfiMmap        = 17,
			EfiBs          = 18,
			Efi32Ih        = 19,
			Efi64Ih        = 20,
			LoadBaseAddr   = 21,
		};
		Type type;
		u32  size;
		// parses a section, and assigns it to the multiboot structure.
		// advances the nextTag pointer by size.
		// only the End tag returns false, everyone else returns true.
		bool parse(Multiboot2 *multiboot);
	};

	// There can be multiple modules present
	struct Module {
		Tag   tag;
		u32   mod_start;
		u32   mod_end;
		char *cmdline() const {
			return ((char *)this + 1);
		}
		void dump() const;
	};

	struct BasicMeminfo {
		Tag  tag;
		u32  mem_lower;
		u32  mem_upper;
		void dump() const;
	};

	struct Bootdev {
		Tag  tag;
		u32  biosdev;
		u32  slice;
		u32  part;
		void dump() const;
	};

	struct Mmap {
		Tag tag;
		u32 entry_size;
		u32 entry_version;

		struct Entry {
			u64 addr;
			u64 len;

			enum Type : u32 {
				Available       = 1,
				Reserved        = 2,
				AcpiReclaimable = 3,
				Nvs             = 4,
				BadRam          = 5
			};
			Type type;
			u32  zero;
		};

		Entry *entries() const {
			return (Entry *)(this + 1);
		}
		void dump() const;
	};

	struct Vbe {
		Tag tag;

		u16 mode;
		u16 interface_seg;
		u16 interface_off;
		u16 interface_len;

		struct ControlInfo {
			u32 signature;    // == "VESA"
			u16 version;      // == 0x0300 for VBE 3.0
			u32 oemStringPtr; // isa vbeFarPtr
			u32 capabilities;
			u32 videoModePtr;  // isa vbeFarPtr
			u16 totalMemory;   // as # of 64KB blocks
			u8  reserved[492]; // 512 bytes total size, probably the later parts
			                   // also contain more modes
		} __attribute__((packed));

		struct ModeInfo {
			u16 attributes;  // deprecated, only bit 7 should be of
			                 // interest to you, and it indicates the mode
			                 // supports a linear frame buffer.
			u8  window_a;    // deprecated
			u8  window_b;    // deprecated
			u16 granularity; // deprecated; used while calculating bank
			                 // numbers
			u16 window_size;
			u16 segment_a;
			u16 segment_b;
			u32 win_func_ptr; // deprecated; used to switch banks from
			                  // protected mode without returning to real
			                  // mode
			u16 pitch;        // number of bytes per horizontal line
			u16 width;        // width in pixels
			u16 height;       // height in pixels
			u8  w_char;       // unused...
			u8  y_char;       // ...
			u8  planes;
			u8  bpp;   // bits per pixel in this mode
			u8  banks; // deprecated; total number of banks in this mode
			u8  memory_model;
			u8  bank_size; // deprecated; size of a bank, almost always 64
			               // KB but may be 16 KB...
			u8 image_pages;
			u8 reserved0;

			u8 red_mask;
			u8 red_position;
			u8 green_mask;
			u8 green_position;
			u8 blue_mask;
			u8 blue_position;
			u8 reserved_mask;
			u8 reserved_position;
			u8 direct_color_attributes;

			u32 framebuffer; // physical address of the linear frame
			                 // buffer; write here to draw to the screen
			u32 off_screen_mem_off;
			u16 off_screen_mem_size; // size of memory in the framebuffer
			                         // but not being displayed on the
			                         // screen
			u8 reserved1[206];
		};

		ControlInfo control_info;
		ModeInfo    mode_info;
		void        dump() const;
	};

	struct Framebuffer {
		Tag tag;

		enum Type : u8 { Indexed = 0, RGB = 1, Text = 2 };

		u64  addr;
		u32  pitch;
		u32  width;
		u32  height;
		u8   bpp;
		Type type;
		u16  reserved;

		union {
			struct {
				u16   palette_num_colors;
				Color palette; // this should be an array
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
		void dump() const;
	};

	struct ElfSections {
		u32   type;
		u32   size;
		u32   num;
		u32   entsize;
		u32   shndx;
		char *sections() const {
			return (char *)(this + 1);
		}
		void dump() const;
	};

	struct Apm {
		Tag  tag;
		u16  version;
		u16  cseg;
		u32  offset;
		u16  cseg_16;
		u16  dseg;
		u16  flags;
		u16  cseg_len;
		u16  cseg_16_len;
		u16  dseg_len;
		void dump() const;
	};

	struct Efi32 {
		Tag  tag;
		u32  pointer;
		void dump() const;
	};

	struct Efi64 {
		Tag  tag;
		u64  pointer;
		void dump() const;
	};

	struct SMBios {
		Tag tag;
		u8  major;
		u8  minor;
		u8  reserved[6];
		u8 *tables() const {
			return (u8 *)(this + 1);
		}
		void dump() const;
	};

	struct AcpiOld {
		Tag tag;
		u8 *rsdp() const {
			return (u8 *)(this + 1);
		}
		void dump() const;
	};

	struct AcpiNew {
		Tag tag;
		u8 *rsdp() const {
			return (u8 *)(this + 1);
		}
		void dump() const;
	};

	struct Network {
		Tag tag;
		u8 *dhcpack() const {
			return (u8 *)(this + 1);
		}
		void dump() const;
	};

	struct EfiMmap {
		Tag tag;
		u32 descr_size;
		u32 descr_vers;
		u8 *efi_mmap() const {
			return (u8 *)(this + 1);
		}
		void dump() const;
	};

	struct Efi32Ih {
		Tag  tag;
		u32  pointer;
		void dump() const;
	};

	struct Efi64Ih {
		Tag  tag;
		u64  pointer;
		void dump() const;
	};

	struct LoadBaseAddr {
		Tag  tag;
		u32  load_base_addr;
		void dump() const;
	};

	// Virtual address pointers to each section, NULL sigifies not present
	AcpiNew *     acpiNew;
	AcpiOld *     acpiOld;
	Apm *         apm;
	BasicMeminfo *basicMemInfo;
	Bootdev *     bootdev;
	const char *  bootloaderName;
	const char *  cmdline;
	EfiMmap *     efiMmap;
	Efi32 *       efi32;
	Efi64 *       efi64;
	Efi32Ih *     efi32ih;
	Efi64Ih *     efi64ih;
	ElfSections * elfSections;
	Framebuffer * framebuffer;
	LoadBaseAddr *loadBaseAddr;
	Mmap *        mmap;
	Module *      modules[256]; // multiple modules can be present
	u32           moduleCount;
	Network *     network;
	SMBios *      smbios;
	Vbe *         vbe;
	// address of the next tag to be parsed
	Tag *nextTag;

	// address of the multiboot section
	static Multiboot2 parse(uptr addr);
};

#endif
