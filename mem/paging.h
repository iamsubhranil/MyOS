#pragma once

#include <boot/multiboot.h>
#include <misc/option.h>
#include <sys/myos.h>
#include <sys/system.h>

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

		static void init(Multiboot *boot);

		static void set(uptr addr);
		static void clear(uptr addr);
		static bool test(uptr addr);

		static bool findFirstFreeFrame(uptr &freeFrame, uptr lastFrame = 0);
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
		// this sets up a frame at the specified physical address,
		// does not toggle any Frame bit
		uptr allocDMA(bool isKernel, bool isWritable, uptr physAddr);
		void free();

		u32 dump() const;
	} __attribute__((packed));

	struct Table {
		Page pages[PagesPerTable];

		// we need to know the idx of the table to calculate
		// the virtual address of the source page and do
		// the memcpy.
		// pageCopyTemp contains the address of a page on
		// source directory, using which a new frame is
		// allocated. the source page is copied on the new
		// frame, and then the dest page just points to the
		// new frame, resetting frame of the temp page.
		// tempAddr contains the address where the temp
		// page points to.
		Table *clone(uptr &phys, siz table_idx, Page *pageCopyTemp,
		             uptr tempAddr) const;
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

		static Directory *CurrentDirectory;
		static Directory *KernelDirectory;

		Directory *clone();

		void dump() const;
		uptr getPhysicalAddress(uptr virtualAddress) const;
	};

	static constexpr siz getTableIndex(uptr address) {
		return (address / (PageSize * PagesPerTable));
	}
	static constexpr siz getPageNo(uptr address) {
		return (address / PageSize) & (PagesPerTable - 1);
	}
	static void  init(Multiboot *boot);
	static void  switchPageDirectory(Directory *newDirectory);
	static Page *getPage(uptr address, bool createIfAbsent, Directory *dir);
	// get a free page from the given directory. the address to which
	// the page points will be set on 'address'.
	// if physicalAddress is specified, the allocated page will point
	// to the given physicalAddress. It doesn't however check whether
	// the frame is empty or not. So be careful while releasing such
	// a frame
	static Page *getFreePage(Directory *dir, uptr &virtualAddress,
	                         Option<uptr> physicalAddress = {});
	// unmaps the page within which the address
	// resides, also invalidates the page in tlb.
	// if the table for the page does not exist, it does nothing.
	// if it is a soft reset, it does not call Frame::free, assuming
	// that this frame may be referenced by another page.
	static void resetPage(uptr address, Directory *dir, bool soft = false);

	static void handlePageFault(Register *r);

	static uptr
	    getPhysicalAddress(uptr       virtualAddress,
	                       Directory *dir = Directory::CurrentDirectory);
};
