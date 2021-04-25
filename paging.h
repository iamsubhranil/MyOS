#pragma once

#include "myos.h"
#include "system.h"

struct Paging {
	static const u32 PageSize           = 0x1000; // 4KiB
	static const u32 PagesPerTable      = 1024;
	static const u32 TablesPerDirectory = 1024;

	static constexpr bool isAligned(uptr addr) {
		return (addr & ~(PageSize - 1)) == 0;
	}

	struct Frame {
		static u32 *frames; // bitset of active frames
		static u32  numberOfFrames;

		static constexpr u32 index(u32 frame) {
			return (frame >>
			        5); // each bit value holds 32 bits, so we get the index
		}
		static constexpr u32 offset(u32 frame) {
			return (frame &
			        31); // each bit value holds 32 bits, so we get the offset
		}

		static void init();

		static void set(u32 addr);
		static void clear(u32 addr);
		static bool test(u32 addr);

		static u32 findFirstFreeFrame();
	};

	// 'unused' crosses byte boundary, but we disable the warning in Makefile
	struct Page {
		u8  present : 1;  // Page present in memory
		u8  rw : 1;       // Read-only if clear, readwrite if set
		u8  user : 1;     // Supervisor level only if clear
		u8  accessed : 1; // Has the page been accessed since last refresh?
		u8  dirty : 1;    // Has the page been written to since last refresh?
		u8  unused : 7;   // Amalgamation of unused and reserved bits
		u32 frame : 20;   // Frame address (shifted right 12 bits)

		void alloc(bool isKernel, bool isWritable);
		void free();

		u32 dump() const;
	} __attribute__((packed));

	struct Table {
		Page pages[PagesPerTable];
	};

	struct Directory {
		Table *tables[TablesPerDirectory];
		/*
		    Array of pointers to the pagetables above, but gives their
		   *physical* location, for loading into the CR3 register.
		*/
		u32 tablesPhysical[TablesPerDirectory];
		/*
		    The physical address of tablesPhysical. This comes into play
		    when we get our kernel heap allocated and the directory
		    may be in a different location in virtual memory.
		*/
		u32 physicalAddr;
	};

	static Directory *currentDirectory;

	static void  init();
	static void  switchPageDirectory(Directory *newDirectory);
	static Page *getPage(u32 address, bool createIfAbsent, Directory *dir);

	static void handlePageFault(Register *r);
};
