#include "paging.h"
#include "isr.h"
#include "kernel.h"
#include "string.h"
#include "terminal.h"

u32 *              Paging::Frame::frames         = NULL;
u32                Paging::Frame::numberOfFrames = 0;
Paging::Directory *Paging::currentDirectory      = NULL;

void Paging::Frame::set(u32 addr) {
	u32 frame = addr / Paging::PageSize;
	frames[index(frame)] |= (1 << offset(frame));
}

void Paging::Frame::clear(u32 addr) {
	u32 frame = addr / Paging::PageSize;
	frames[index(frame)] &= ~((u32)1 << offset(frame));
}

bool Paging::Frame::test(u32 addr) {
	u32 frame = addr / Paging::PageSize;
	return frames[index(frame)] & (1 << offset(frame));
}

u32 Paging::Frame::findFirstFreeFrame() {
	for(u32 i = 0; i < index(numberOfFrames); i++) {
		if(frames[i] != 0xFFFFFFFF) { // nothing free, exit early.
			// at least one bit is free here.
			for(u32 j = 0; j < 32; j++) {
				u32 toTest = 0x1 << j;
				if(!(frames[i] & toTest)) {
					// 32 bits in one u32
					return i * 32 + j; // return the index of the set bit
				}
			}
		}
	}
	return -1;
}

u32 Paging::Page::dump() const {
	u32 res = Terminal::write("Page (frame: ", frame, " ");
	if(present) {
		res += Terminal::write("present ");
	}
	if(rw) {
		res += Terminal::write("read-only ");
	}
	if(user) {
		res += Terminal::write("user-mode ");
	}
	res += Terminal::write(")");
	return res;
}

void Paging::Page::alloc(bool isKernel, bool isWritable) {
	// Terminal::write(*this);
	if(frame) {
		// Terminal::write(" -> No alloc!\n");
		return;
	}
	u32 idx = Frame::findFirstFreeFrame();
	if(idx == (u32)-1) {
		// Terminal::err("No free frames!");
		for(;;)
			;
	}
	Frame::set(idx * 0x1000);
	present = 1;
	rw      = isWritable;
	user    = !isKernel;
	frame   = idx;
	// Terminal::write(" -> Alloc to frame ", frame, "\n");
}

void Paging::Page::free() {
	if(!frame)
		return;
	Frame::clear(frame); // shouldn't this also be multipled by 0x1000?
	frame = 0;
}

void Paging::Frame::init() {
	numberOfFrames = Kernel::Memory::Size / PageSize;
	frames         = (u32 *)Kernel::Memory::alloc(index(numberOfFrames));
	memset(frames, 0, index(numberOfFrames));
}

void Paging::switchPageDirectory(Paging::Directory *dir) {
	currentDirectory = dir;
	asm volatile("mov %0, %%cr3" ::"r"(&dir->tablesPhysical));
	u32 cr0;
	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 |= 0x80000000; // Enable paging!
	asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

Paging::Page *Paging::getPage(u32 address, bool create,
                              Paging::Directory *dir) {
	// Terminal::write(Terminal::Mode::Hex, "Address: ", address,
	//                Terminal::Mode::Reset);

	// Turn the address into an index.
	// Find the page table containing this address.
	u32 table_idx = address / (PageSize * PagesPerTable);
	u32 pageno    = (address / PageSize) & (PagesPerTable - 1);

	// Terminal::write("  TableIdx: ", table_idx, " PageNo: ", pageno, " ",
	//                address / PageSize, " ", PagesPerTable - 1,
	//                Terminal::Mode::Reset, "\n");

	if(dir->tables[table_idx]) { // If this table is already assigned
		// Terminal::write(table_idx, " Page already exists!\n");
		return &dir->tables[table_idx]->pages[pageno];
	} else if(create) {
		u32 tmp;
		dir->tables[table_idx] = (Paging::Table *)Kernel::Memory::alloc_a(
		    sizeof(Paging::Table), &tmp);
		// Terminal::write(
		//   "Creating new table (sizeof(Table): ", sizeof(Paging::Table),
		//   " sizeof(Page): ", sizeof(Paging::Page),
		//   " phyaddr: ", Terminal::Mode::Hex, tmp, Terminal::Mode::Reset,
		//   "\n");
		memset(dir->tables[table_idx], 0, sizeof(Paging::Table));
		dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
		return &dir->tables[table_idx]->pages[pageno];
	} else {
		return NULL;
	}
}

void Paging::handlePageFault(Register *regs) {
	// A page fault has occurred.
	// The faulting address is stored in the CR2 register.
	u32 faulting_address;
	asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

	// The error code gives us details of what happened.
	int present = !(regs->err_code & 0x1); // Page not present
	int rw      = regs->err_code & 0x2;    // Write operation?
	int us      = regs->err_code & 0x4;    // Processor was in user-mode?
	int reserved =
	    regs->err_code & 0x8; // Overwritten CPU-reserved bits of page entry?
	int id = regs->err_code & 0x10; // Caused by an instruction fetch?

	// Output an error message.
	Terminal::write("Page fault! ( ");
	if(present) {
		Terminal::write("present ");
	}
	if(rw) {
		Terminal::write("read-only ");
	}
	if(us) {
		Terminal::write("user-mode ");
	}
	if(reserved) {
		Terminal::write("reserved ");
	}
	if(id) {
		Terminal::write("ifetch");
	}
	Terminal::write(") at ", Terminal::Mode::Hex, faulting_address,
	                Terminal::Mode::Reset, "\n");
	for(;;)
		;
}

void Paging::init() {
	Terminal::info("Setting up paging..");

	Terminal::prompt(VGA::Color::Brown, "Paging", "Setting up frames..");
	Frame::init();

	Terminal::prompt(VGA::Color::Brown, "Paging",
	                 "Creating kernel page directory..");
	Directory *kernelDirectory =
	    (Directory *)Kernel::Memory::alloc_a(sizeof(Directory));
	memset(kernelDirectory, 0, sizeof(Directory));

	currentDirectory = kernelDirectory;

	Terminal::prompt(VGA::Color::Brown, "Paging", "Allocating kernel pages..");
	u32 i = 0;
	while(i < Kernel::Memory::placementAddress) {
		getPage(i, true, kernelDirectory)
		    ->alloc(false, false); // isKernel is set to false, why?
		i += Paging::PageSize;
	}

	Terminal::prompt(VGA::Color::Brown, "Paging",
	                 "Setting up page fault handler..");
	ISR::installHandler(14, handlePageFault);

	Terminal::prompt(VGA::Color::Brown, "Paging", "Switching page directory..");
	switchPageDirectory(kernelDirectory);
	Terminal::done("Paging setup complete!");
}
