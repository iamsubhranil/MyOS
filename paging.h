#pragma once

#include "myos.h"
#include "system.h"

struct Paging {
	static const siz PageSize           = 0x1000; // 4KiB
	static const siz PagesPerTable      = 1024;
	static const siz TablesPerDirectory = 1024;

	static constexpr bool isAligned(uptr addr) {
		return (addr & (PageSize - 1)) == 0;
	}

	static constexpr void alignAddress(uptr &addr) {
		addr &= ~(PageSize - 1);
		addr += PageSize;
	}

	static constexpr void alignIfNeeded(uptr &addr) {
		if(!isAligned(addr))
			alignAddress(addr);
	}

	struct Frame {
		static u32 *frames; // bitset of active frames
		static siz  numberOfFrames;
		static siz  numberOfSets; // number of values in 'frames' array

		static constexpr siz index(uptr frame) {
			return (frame >>
			        5); // each bit value holds 32 bits, so we get the index
		}
		static constexpr siz offset(uptr frame) {
			return (frame &
			        31); // each bit value holds 32 bits, so we get the offset
		}

		static void init();

		static void set(uptr addr);
		static void clear(uptr addr);
		static bool test(uptr addr);

		static uptr findFirstFreeFrame(uptr lastFrame = 0);
		// searches in the given range (inclusive in to_index, exclusive in
		// to_offset)
		static bool searchInRange(uptr from_index, uptr to_index, uptr &result);
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

		// optionally takes the last allocated frame index to pass
		// to findFirstFreeFrame, so that it does not start searching
		// from the beginning.
		// returns the allocated frame
		uptr alloc(bool isKernel, bool isWritable, uptr lastFrame = 0);
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
		uptr tablesPhysical[TablesPerDirectory];
		/*
		    The physical address of tablesPhysical. This comes into play
		    when we get our kernel heap allocated and the directory
		    may be in a different location in virtual memory.
		*/
		siz physicalAddr;

		static Directory *currentDirectory;
		static Directory *kernelDirectory;
	};

	static void  init();
	static void  switchPageDirectory(Directory *newDirectory);
	static Page *getPage(uptr address, bool createIfAbsent, Directory *dir);

	static void handlePageFault(Register *r);
};
