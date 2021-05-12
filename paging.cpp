#include "paging.h"
#include "asm.h"
#include "heap.h"
#include "isr.h"
#include "kernel_layout.h"
#include "memory.h"
#include "stacktrace.h"
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

bool Paging::Frame::findFirstFreeFrame(uptr &result, uptr lastFrame) {
	siz lastIndex = index(lastFrame);
	if(searchInRange(lastIndex, numberOfSets, result)) {
		return true;
	}

	// if we already started searching from the beginning, we're done
	if(lastIndex == 0)
		return false;

	// else, search from the beginning
	if(searchInRange(0, lastIndex, result)) {
		return true;
	}
	return false;
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
	siz idx;
	if(!Frame::findFirstFreeFrame(idx, lastFrame)) {
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
	frame   = 0;
	present = 0;
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
	// set up new directory
	Directory::CurrentDirectory = dir;
	uptr tableloc               = (uptr)V2P(&dir->tablesPhysical);
	asm volatile("mov %0, %%cr3" ::"r"(tableloc));
}

Paging::Page *Paging::getPage(uptr address, bool create,
                              Paging::Directory *dir) {
	// Terminal::write(Terminal::Mode::Hex, "Address: ", address, " ");

	// Turn the address into an index.
	// Find the page table containing this address.
	// find the overall page no
	siz table_idx = getTableIndex(address);
	siz pageno    = getPageNo(address);

	// Terminal::write("  TableIdx: ", table_idx, " PageNo: ", pageno, " ",
	//                address / PageSize, Terminal::Mode::Reset, "\n");

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

void Paging::resetPage(uptr address, Paging::Directory *dir, bool soft) {
	siz table_idx = getTableIndex(address);
	siz pageno    = getPageNo(address);
	if(dir->tables[table_idx]) {
		if(!soft) {
			dir->tables[table_idx]->pages[pageno].free();
		} else {
			dir->tables[table_idx]->pages[pageno].frame   = 0;
			dir->tables[table_idx]->pages[pageno].present = 0;
		}
		Asm::invlpg(address);
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
	return table;
}

void Paging::Directory::dump() const {
	for(siz i = 0; i < TablesPerDirectory; i++) {
		if(tables[i]) {
			siz j = 0;
			while(1) {
				while(j < PagesPerTable && tables[i]->pages[j].present == 0)
					j++;
				if(j == PagesPerTable)
					break;
				siz rangeStart = j;
				while(j < PagesPerTable && tables[i]->pages[j].present) j++;
				siz rangeEnd = j;
				Terminal::write(
				    Terminal::Mode::Hex,
				    ((i * PagesPerTable) + (rangeStart)) * Paging::PageSize,
				    " - ", ((i * PagesPerTable) + (rangeEnd)) * PageSize, ", ",
				    Terminal::Mode::Reset);
			}
		}
	}
	Terminal::write("\n");
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
	Stacktrace::print();
	for(;;)
		;
}

void Paging::init() {
	Terminal::info("Setting up paging..");
	PROMPT_INIT("Paging", Brown);
	PROMPT("Setting up frames..");
	Frame::init();

	PROMPT("Setting up page fault handler..");
	ISR::installHandler(14, handlePageFault);

	PROMPT("Creating kernel page directory..");
	Directory::KernelDirectory =
	    (Directory *)Memory::alloc_a(sizeof(Directory));
	memset(Directory::KernelDirectory, 0, sizeof(Directory));

	Directory::KernelDirectory->physicalAddr =
	    V2P((uptr)&Directory::KernelDirectory->tablesPhysical);

	PROMPT("Allocating kernel heap..");
	Heap *heap = (Heap *)Memory::alloc_a(sizeof(Heap));

	PROMPT("Allocating pages for kernel heap..");
	uptr i = 0;
	for(i = Heap::Start; i < Heap::Start + Heap::InitialSize;
	    i += Paging::PageSize) {
		getPage(i, true, Directory::KernelDirectory);
	}

	// We need to create a mapping between our placement address,
	// which is 0xc0000000 as specified in the linker script,
	// with our physical address, which starts from 0.
	// so, we set the lastFrame variable to 0 to ensure that
	// the mapping starts from 0x00000000 frame address.
	// For this reason, kernel memory must be the first thing that
	// is mapped.
	uptr lastFrame = 0;

	PROMPT("Allocating frames for kernel memory and heap..");
	for(i = KMEM_BASE; i < Memory::placementAddress; i += Paging::PageSize) {
		lastFrame = getPage(i, true, Directory::KernelDirectory)
		                ->alloc(true, false, lastFrame);
	}
	for(i = Heap::Start; i < Heap::Start + Heap::InitialSize;
	    i += Paging::PageSize) {
		lastFrame = getPage(i, false, Directory::KernelDirectory)
		                ->alloc(true, false, lastFrame);
	}

	PROMPT("Switching page directory..");
	switchPageDirectory(Directory::KernelDirectory);

	PROMPT("Initalizing kernel heap..");
	Memory::kernelHeap = heap;
	Memory::kernelHeap->init(Heap::Start, Heap::Start + Heap::InitialSize,
	                         Heap::Start + Heap::InitialSize, false, false);

	PROMPT("Dumping kernel directory: ");
	Directory::KernelDirectory->dump();
	PROMPT("Cloning kernel directory..");
	Directory::CurrentDirectory = Directory::KernelDirectory->clone();

	Terminal::done("Paging setup complete!");
}
