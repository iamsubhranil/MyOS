#include "paging.h"
#include "asm.h"
#include "heap.h"
#include "isr.h"
#include "memory.h"
#include "string.h"
#include "terminal.h"

u32 *              Paging::Frame::frames               = NULL;
siz                Paging::Frame::numberOfFrames       = 0;
siz                Paging::Frame::numberOfSets         = 0;
Paging::Directory *Paging::Directory::CurrentDirectory = NULL;
Paging::Directory *Paging::Directory::KernelDirectory  = NULL;

void Paging::Frame::set(uptr addr) {
	uptr frame = addr / Paging::PageSize;
	frames[index(frame)] |= ((u32)1 << offset(frame));
}

void Paging::Frame::clear(uptr addr) {
	uptr frame = addr / Paging::PageSize;
	frames[index(frame)] &= ~((u32)1 << offset(frame));
}

bool Paging::Frame::test(uptr addr) {
	uptr frame = addr / Paging::PageSize;
	return frames[index(frame)] & ((u32)1 << offset(frame));
}

bool Paging::Frame::searchInRange(uptr fri, uptr toi, uptr &result) {
	result            = 0;
	const u32 allFull = Limits::U32Max;
	for(uptr i = fri; i < toi; i++) {
		if(frames[i] == allFull)
			continue;
		result = i * sizeof(siz) * 8;
		if(frames[i] == 0)
			return true;
		u32 search = ~frames[i]; // we are assuming siz is 32 bit
		result += Asm::bsf(search);
		return true;
	}
	return false;
}

uptr Paging::Frame::findFirstFreeFrame(uptr lastFrame) {
	siz  lastIndex = index(lastFrame);
	uptr frame     = 0;
	if(searchInRange(lastIndex, numberOfSets, frame)) {
		return frame;
	}

	// if we already started searching from the beginning, we're done
	if(lastIndex == 0)
		return -1;

	// else, search from the beginning
	if(searchInRange(0, lastIndex, frame)) {
		return frame;
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

uptr Paging::Page::alloc(bool isKernel, bool isWritable, uptr lastFrame) {
	// Terminal::write(*this);
	if(frame) {
		// Terminal::write(" -> No alloc!\n");
		return frame;
	}
	siz idx = Frame::findFirstFreeFrame(lastFrame);
	if(idx == (siz)-1) {
		Terminal::err("No free frames!");
		for(;;)
			;
	}
	Frame::set(idx * Paging::PageSize);
	present = 1;
	rw      = isWritable;
	user    = !isKernel;
	frame   = idx;
	// Terminal::write(" -> Alloc to frame ", frame, "\n");
	return frame;
}

void Paging::Page::free() {
	if(!frame)
		return;
	Frame::clear(frame * Paging::PageSize);
	frame = 0;
}

void Paging::Frame::init() {
	numberOfFrames = Memory::Size / PageSize;
	// each frame occupies 1 bit of memory, so we need
	// numberOfFrames / 8 bytes of memory
	frames = (u32 *)Memory::alloc(numberOfFrames / 8);
	memset(frames, 0, numberOfFrames / 8);
	numberOfSets = numberOfFrames / (8 * sizeof(frames[0]));
}

void Paging::switchPageDirectory(Paging::Directory *dir) {
	Directory::CurrentDirectory = dir;
	asm volatile("mov %0, %%cr3" ::"r"(&dir->tablesPhysical));
	u32 cr0;
	asm volatile("mov %%cr0, %0" : "=r"(cr0));
	cr0 |= 0x80000000; // Enable paging!
	asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

Paging::Page *Paging::getPage(uptr address, bool create,
                              Paging::Directory *dir) {
	// Terminal::write(Terminal::Mode::HexOnce, "Address: ", address, " ");

	// Turn the address into an index.
	// Find the page table containing this address.
	siz table_idx = address / (PageSize * PagesPerTable);
	siz pageno    = (address / PageSize) & (PagesPerTable - 1);

	// Terminal::write("  TableIdx: ", table_idx, " PageNo: ", pageno, " ",
	//                address / PageSize, " ", PagesPerTable - 1,
	//                Terminal::Mode::Reset, "\n");

	if(dir->tables[table_idx]) { // If this table is already assigned
		// Terminal::write(table_idx, " Page already exists!\n");
		return &dir->tables[table_idx]->pages[pageno];
	} else if(create) {
		uptr tmp;
		dir->tables[table_idx] =
		    (Paging::Table *)Memory::alloc_a(sizeof(Paging::Table), tmp);
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

Paging::Directory *Paging::Directory::clone() const {
	uptr       phys;
	Directory *dir = (Directory *)Memory::alloc_a(sizeof(Directory), phys);
	memset(dir, 0, sizeof(Directory));

	uptr offset       = (uptr)&dir->tablesPhysical - (uptr)dir;
	dir->physicalAddr = phys + offset;

	for(siz i = 0; i < Paging::TablesPerDirectory; i++) {
		if(!tables[i])
			continue;
		// check if the table corresponds to a table
		// in the kernel directory. if it does,
		// we're not gonna copy it, we're gonna
		// directly link it.
		if(KernelDirectory->tables[i] == tables[i]) {
			dir->tables[i]         = tables[i];
			dir->tablesPhysical[i] = tablesPhysical[i];
		} else {
			uptr phys;
			dir->tables[i]         = tables[i]->clone(phys);
			dir->tablesPhysical[i] = phys | 0x07; // Present, RW, User
		}
	}

	return dir;
}

Paging::Table *Paging::Table::clone(uptr &phys) const {
	Table *table = (Table *)Memory::alloc_a(sizeof(Table), phys);
	memset(table, 0, sizeof(Table));

	for(siz i = 0; i < Paging::PagesPerTable; i++) {
		if(!pages[i].frame) // unallocated page, don't bother
			continue;
		// alloc a new frame
		table->pages[i].alloc(false, false, 0);

		// clone the flags
		table->pages[i].present  = pages[i].present;
		table->pages[i].rw       = pages[i].rw;
		table->pages[i].user     = pages[i].user;
		table->pages[i].accessed = pages[i].accessed;
		table->pages[i].dirty    = pages[i].dirty;

		// finally, memcpy the data.
		// we shouldn't really need to use assembly and/or
		// disable paging here
		memcpy((void *)(uptr)(table->pages[i].frame * Paging::PageSize),
		       (void *)(uptr)(pages[i].frame * Paging::PageSize),
		       Paging::PageSize);
	}
}

void Paging::handlePageFault(Register *regs) {
	// A page fault has occurred.
	// The faulting address is stored in the CR2 register.
	uptr faulting_address;
	asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

	// The error code gives us details of what happened.
	int present = regs->err_code & 0x1; // Page not present
	int rw      = regs->err_code & 0x2; // Write operation?
	int us      = regs->err_code & 0x4; // Processor was in user-mode?
	int reserved =
	    regs->err_code & 0x8; // Overwritten CPU-reserved bits of page entry?
	int id = regs->err_code & 0x10; // Caused by an instruction fetch?

	// Output an error message.
	Terminal::write("Page fault! ( ");
	if(present) {
		Terminal::write("protection-violation ");
	} else {
		Terminal::write("not-present ");
	}
	if(rw) {
		Terminal::write("write-access ");
	} else {
		Terminal::write("read-access ");
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
	PROMPT_INIT("Paging", Brown);
	PROMPT("Setting up frames..");
	Frame::init();

	PROMPT("Creating kernel page directory..");
	Directory::KernelDirectory =
	    (Directory *)Memory::alloc_a(sizeof(Directory));
	memset(Directory::KernelDirectory, 0, sizeof(Directory));

	Directory::KernelDirectory->physicalAddr =
	    (uptr)&Directory::KernelDirectory->tablesPhysical;

	PROMPT("Allocating kernel heap..");
	Heap *heap = (Heap *)Memory::alloc(sizeof(Heap));

	PROMPT("Allocating pages for kernel heap..");
	uptr i = 0;
	for(i = Heap::Start; i < Heap::Start + Heap::InitialSize;
	    i += Paging::PageSize) {
		getPage(i, true, Directory::KernelDirectory);
	}

	PROMPT("Allocating frames for kernel memory and heap..");
	i              = 0;
	uptr lastFrame = 0;
	while(i < Memory::placementAddress) {
		lastFrame = getPage(i, true, Directory::KernelDirectory)
		                ->alloc(true, false, lastFrame);
		i += Paging::PageSize;
	}

	i = 0;
	for(i = Heap::Start; i < Heap::Start + Heap::InitialSize;
	    i += Paging::PageSize) {
		lastFrame = getPage(i, false, Directory::KernelDirectory)
		                ->alloc(true, false, lastFrame);
	}

	PROMPT("Setting up page fault handler..");
	ISR::installHandler(14, handlePageFault);

	PROMPT("Cloning kernel directory..");
	Directory::CurrentDirectory = Directory::KernelDirectory->clone();

	PROMPT("Switching page directory..");
	switchPageDirectory(Directory::KernelDirectory);

	PROMPT("Initalizing kernel heap..");
	Memory::kernelHeap = heap;
	Memory::kernelHeap->init(Heap::Start, Heap::Start + Heap::InitialSize,
	                         Heap::Start + Heap::MaxSize, false, false);

	Terminal::done("Paging setup complete!");
}
