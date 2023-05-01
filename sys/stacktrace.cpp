#include <arch/x86/kernel_layout.h>
#include <drivers/terminal.h>
#include <mem/memory.h>
#include <sched/scheduler.h>
#include <sys/stacktrace.h>

Multiboot::Elf32::Symtab *symbols     = NULL;
const char               *stringTable = NULL;
u32                       symbolCount = 0;

void sortSymbols() {
	bool swapped = true;
	for(u32 j = 0; j < symbolCount; j++) {
		swapped = false;
		for(u32 i = 0; i < symbolCount - j - 1; i++) {
			if(symbols[i].value > symbols[i + 1].value) {
				u32 ip               = symbols[i].value;
				u32 idx              = symbols[i].name;
				symbols[i].value     = symbols[i + 1].value;
				symbols[i].name      = symbols[i + 1].name;
				symbols[i + 1].value = ip;
				symbols[i + 1].name  = idx;
				swapped              = true;
			}
		}
		if(!swapped)
			break;
	}
}

void Stacktrace::dumpSymbols() {
	for(u32 i = 0; i < symbolCount; i++) {
		Terminal::write(Terminal::Mode::HexOnce, symbols[i].value, " -> ",
		                &stringTable[symbols[i].name], "\n");
	}
}

void Stacktrace::loadSymbols(Multiboot *boot) {
	Multiboot::Elf32::Header *headers =
	    (Multiboot::Elf32::Header *)P2V(boot->addr);
	for(u32 i = 0; i < boot->num; i++) {
		Multiboot::Elf32::Header h = headers[i];
		if(h.type == Multiboot::Elf32::Header::Type::Strtab) {
			if(!stringTable)
				stringTable = (char *)(uptr)h.addr;
		} else if(h.type == Multiboot::Elf32::Header::Type::Symtab) {
			symbols     = (Multiboot::Elf32::Symtab *)(uptr)h.addr;
			symbolCount = h.size / sizeof(Multiboot::Elf32::Symtab);
			sortSymbols();
		}
	}
}

struct SymbolDetails {
	const char *sym;
	uptr        symStart;
};

SymbolDetails getSymbolName(u32 ip) {
	u32 lastIp     = 0;
	u32 lastSymbol = 0;
	for(u32 i = 0; i < symbolCount; i++) {
		if(ip > lastIp && ip <= symbols[i].value) {
			return {&stringTable[lastSymbol], lastIp};
		}
		lastIp     = symbols[i].value;
		lastSymbol = symbols[i].name;
	}
	return {NULL, 0};
}

void Stacktrace::print(void *ebp) {
	Terminal::write("\t\tStack Trace\n");
	Terminal::write("===========================\n");
	Terminal::write("Current task: ", (i32)Scheduler::getCurrentTask()->id,
	                "\n");
	PROMPT_INIT("At", Green);
	StackFrame *stk;
	if(!ebp) {
		asm("mov %%ebp,%0" : "=r"(stk)::);
	} else {
		stk = (StackFrame *)ebp;
	}
	Terminal::write(Terminal::Mode::Dec);
	for(u32 frame = 0; stk; ++frame, stk = (StackFrame *)stk->next) {
		SymbolDetails symdet = getSymbolName(stk->eip);
		if(!symdet.sym) {
			PROMPT("Frame #", frame, " ->  N/A ",
			       "(EIP:", Terminal::Mode::HexOnce, stk->eip,
			       " EBP: ", (void *)stk, ")");
		} else {
			PROMPT("Frame #", frame, " ->  ", symdet.sym, "+",
			       Terminal::Mode::HexOnce, stk->eip - symdet.symStart,
			       "  (EIP:", Terminal::Mode::HexOnce, stk->eip,
			       " EBP: ", (void *)stk, ")");
		}
	}
	Terminal::write("\n\n");
}
