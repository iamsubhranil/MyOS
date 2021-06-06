#include <arch/x86/asm.h>
#include <arch/x86/isr.h>
#include <arch/x86/kernel_layout.h>
#include <drivers/terminal.h>
#include <mem/heap.h>
#include <mem/memory.h>
#include <mem/paging.h>
#include <sys/stacktrace.h>
#include <sys/string.h>

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

uptr Paging::Page::allocDMA(bool isKernel, bool isWritable, uptr physicalAddr) {
	present = 1;
	rw      = isWritable;
	user    = !isKernel;
	frame   = physicalAddr >> 12;
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

void Paging::Frame::init(Multiboot *boot) {
	// calculate total memory
	u8   nummaps  = boot->mmap_length / sizeof(Multiboot::MemoryMap);
	uptr mmap_ptr = (uptr)boot->mmap_addr;
	for(u8 i = 0; i < nummaps; i++, mmap_ptr += sizeof(Multiboot::MemoryMap)) {
		Multiboot::MemoryMap *m = (Multiboot::MemoryMap *)P2V(mmap_ptr);
		if(m->type == Multiboot::MemoryMap::Type::Usable &&
		   m->length >= (1024 * 1024))
			Memory::Size += m->length;
	}
	Terminal::write("Total memory: ", Terminal::Mode::HexOnce, Memory::Size,
	                "\n");
	numberOfFrames = Memory::Size / PageSize;
	// each frame occupies 1 bit of memory, so we need
	// numberOfFrames / 8 bytes of memory
	frames = (u32 *)Memory::alloc(numberOfFrames / 8);
	// by default, mark all frames as used
	memset(frames, 0xFF, numberOfFrames / 8);
	numberOfSets = numberOfFrames / (8 * sizeof(frames[0]));

	// mark the low memory as unused for now, we will map all frames
	// in this range later, as it contains various bootloader infos
	for(siz i = 0; i < 1024 * 1024; i++) Frame::clear(i);

	// check which frames are available for allocation,
	// and set them as free
	mmap_ptr = (uptr)boot->mmap_addr;
	for(u8 i = 0; i < nummaps; i++, mmap_ptr += sizeof(Multiboot::MemoryMap)) {
		Multiboot::MemoryMap *m = (Multiboot::MemoryMap *)P2V(mmap_ptr);
		if(m->base_addr < 0x100000) {
			continue;
		}
		if(m->type == Multiboot::MemoryMap::Type::Usable &&
		   m->length >= (1024 * 1024)) {
			// this block is usable and we have at least 1 MiB of free
			// space, so we can use this
			for(u64 j = m->base_addr, l = 0; l < m->length;
			    l += Paging::PageSize) {
				Frame::clear((j + l) & 0xFFFFFFFF);
				// Terminal::write(l, "\n");
			}
		}
	}
}

void Paging::switchPageDirectory(Paging::Directory *dir) {
	Asm::cr3_store((uptr)dir->physicalAddr);
	Directory::CurrentDirectory = dir;
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
		dir->tables[table_idx] =
		    (Paging::Table *)Memory::alloc_a(sizeof(Paging::Table));
		uptr tmp = Paging::getPhysicalAddress((uptr)dir->tables[table_idx]);
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

Paging::Page *Paging::getFreePage(Paging::Directory *dir, uptr &address,
                                  Option<uptr> physicalAddress) {
	Page *p = NULL;
	// first, check for a free page on already allocated tables
	for(siz i = 0; i < Paging::TablesPerDirectory; i++) {
		if(!dir->tables[i])
			continue;
		// check for a free page
		Table *t = dir->tables[i];
		for(siz j = 0; j < Paging::PagesPerTable; j++) {
			if(t->pages[j].frame == 0) { // it is free, so allocate and return
				                         // this. we actually need to alloc here
				                         // to mark it as used.
				p       = &t->pages[j];
				address = ((i * Paging::PagesPerTable) + j) * Paging::PageSize;
				break;
			}
		}
	}
	if(!p) {
		// no allocated table contains a free page, so alloc a new table
		for(siz i = 0; i < Paging::TablesPerDirectory; i++) {
			if(!dir->tables[i]) {
				// just call getPage(addr), that will automatically allocate
				// a new table
				address = ((i * Paging::PagesPerTable)) * Paging::PageSize;
				p       = getPage(address, true, dir);
				break;
			}
		}
	}
	if(!p)
		return NULL;
	if(!physicalAddress.has) {
		p->alloc(false, true);
	} else {
		p->frame    = physicalAddress.value;
		p->present  = 1;
		p->rw       = 1;
		p->user     = 1;
		p->dirty    = 0;
		p->accessed = 0;
		Frame::set(physicalAddress.value);
	}
	return p;
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

Paging::Directory *Paging::Directory::clone() {
	Directory *dir = (Directory *)Memory::alloc_a(sizeof(Directory));
	memset(dir, 0, sizeof(Directory));

	dir->physicalAddr = Paging::getPhysicalAddress((uptr)&dir->tablesPhysical);
	// get a free page on present directory to act as a
	// temp page pointing to the dest frame.
	// it already allocates the page.
	uptr  pageCopyAddress = 0;
	Page *pageCopyTemp    = getFreePage(this, pageCopyAddress);
	if(!pageCopyTemp) {
		Terminal::err("No free page on present table to create a copy!");
		Stacktrace::print();
		for(;;)
			;
	}

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
			dir->tables[i] =
			    tables[i]->clone(phys, i, pageCopyTemp, pageCopyAddress);
			dir->tablesPhysical[i] = phys | 0x07; // Present, RW, User
		}
	}

	// release the temporary page
	Paging::resetPage(pageCopyAddress, this);
	// the temporary page is also copied to the dest directory.
	// who ever is pointing to that now, release it too
	dir->tables[getTableIndex(pageCopyAddress)]
	    ->pages[getPageNo(pageCopyAddress)]
	    .free();

	return dir;
}

Paging::Table *Paging::Table::clone(uptr &phys, siz table_idx,
                                    Page *pageCopyTemp,
                                    uptr  pageCopyAddress) const {
	Table *table = (Table *)Memory::alloc_a(sizeof(Table));
	memset(table, 0, sizeof(Table));
	phys = Paging::getPhysicalAddress((uptr)table);

	for(siz i = 0; i < Paging::PagesPerTable; i++) {
		if(!pages[i].frame) { // unallocated page, don't bother
			continue;
		}
		// the temp page already contains an allocated frame,
		// so just memcpy from source page to the temp page
		memcpy((void *)(uptr)((table_idx * Paging::PagesPerTable + i) *
		                      Paging::PageSize),
		       (void *)(uptr)(pageCopyAddress), Paging::PageSize);

		// copy the frame
		table->pages[i].frame = pageCopyTemp->frame;
		// reset the frame of the temporary page
		pageCopyTemp->frame = 0;
		// allocate a new frame for next iteration
		pageCopyTemp->alloc(false, true);

		// clone the flags
		table->pages[i].present  = pages[i].present;
		table->pages[i].rw       = pages[i].rw;
		table->pages[i].user     = pages[i].user;
		table->pages[i].accessed = pages[i].accessed;
		table->pages[i].dirty    = pages[i].dirty;
	}
	return table;
}

void Paging::Directory::dump() const {
	for(siz i = 0; i < TablesPerDirectory; i++) {
		if(tables[i]) {
			siz j = 0;
			while(1) {
				while(j < PagesPerTable && tables[i]->pages[j].frame == 0) j++;
				if(j == PagesPerTable)
					break;
				Terminal::write("First present page: ", j, "\n");
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

uptr Paging::getPhysicalAddress(uptr virtualAddress, Paging::Directory *dir) {
	if(!dir)
		return V2P(virtualAddress);
	return dir->getPhysicalAddress(virtualAddress);
}

uptr Paging::Directory::getPhysicalAddress(uptr virtualAddress) const {
	siz  tbl           = getTableIndex(virtualAddress);
	siz  page          = getPageNo(virtualAddress);
	uptr physicalStart = tables[tbl]->pages[page].frame * Paging::PageSize;
	physicalStart += (virtualAddress) & (Paging::PageSize - 1);
	return physicalStart;
}

void Paging::init(Multiboot *boot) {
	PROMPT_INIT("Paging", Orange);
	PROMPT("Setting up paging..");
	Frame::init(boot);

	PROMPT("Setting up page fault handler..");
	ISR::installHandler(14, handlePageFault);

	PROMPT("Creating kernel page directory..");
	Directory::KernelDirectory =
	    (Directory *)Memory::alloc_a(sizeof(Directory));
	memset(Directory::KernelDirectory, 0, sizeof(Directory));

	Directory::KernelDirectory->physicalAddr =
	    getPhysicalAddress((uptr)&Directory::KernelDirectory->tablesPhysical);

	for(uptr i = Heap::KHeapStart; i < Heap::KHeapEnd; i += Paging::PageSize) {
		getPage(i, true, Directory::KernelDirectory);
	}
	uptr lastFrame = 0;

	// check if fb is available and map accordingly
	if(boot->flags & 0x800) {
		PROMPT("VBE is available! Mapping the framebuffer!");
		// map the vbe framebuffer
		Multiboot::VbeModeInfo *vbe =
		    (Multiboot::VbeModeInfo *)P2V(boot->vbe_mode_info);

		uptr fbaddr = vbe->physbase;
		uptr fbend  = fbaddr + vbe->pitch * vbe->Yres;
		// identity map the fb
		for(uptr i = fbaddr; i < fbend; i += Paging::PageSize) {
			// use allocDMA, since we need the page at that particular
			// physical address specifically
			getPage(i, true, Directory::KernelDirectory)
			    ->allocDMA(true, true, i);
		}
	}
	// We need to create a mapping between our placement address,
	// which is 0xc0000000 as specified in the linker script,
	// with our physical address, which starts from 0.
	// so, we set the lastFrame variable to 0 to ensure that
	// the mapping starts from 0x00000000 frame address.
	// For this reason, kernel memory must be the first thing that
	// is mapped.
	lastFrame = 0;

	// PROMPT("Allocating frames for kernel memory and heap..");
	for(uptr i = KMEM_BASE; i < Memory::placementAddress;
	    i += Paging::PageSize) {
		lastFrame = getPage(i, true, Directory::KernelDirectory)
		                ->alloc(true, true, lastFrame);
		// Terminal::write("lastframe: ", Terminal::Mode::HexOnce, lastFrame,
		//                "\n");
	}
	// map the first page
	getPage(Heap::KHeapStart, true, Directory::KernelDirectory)
	    ->alloc(true, true, lastFrame);
	// we don't need to map heap

	PROMPT("Dumping kernel directory: ");
	Directory::KernelDirectory->dump();
	PROMPT("Switching page directory..");
	switchPageDirectory(Directory::KernelDirectory);

	// if vbe is available, switch to it now
	if(boot->flags & 0x800) {
		PROMPT("Switching to VGA..");
		// switch the terminal to VGA
		Terminal::write(Terminal::Output::VGA);
		Terminal::init(boot);
		PROMPT("Switched to VGA from Serial..");
	}

	PROMPT("Initalizing kernel heap..");
	Heap *heap = (Heap *)(uptr)(Heap::KHeapStart);
	heap->init(Heap::KHeapEnd - Heap::KHeapStart);
	Memory::kernelHeap = heap;

	Terminal::done("Paging setup complete!");
}
