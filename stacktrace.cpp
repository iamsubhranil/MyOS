#include "stacktrace.h"
#include "terminal.h"

void Stacktrace::print() {
	Terminal::write("\t\tStack Trace\n");
	Terminal::write("===========================\n");
	PROMPT_INIT("At", Green);
	StackFrame *stk;
	asm("mov %%ebp,%0" : "=r"(stk)::);
	Terminal::write(Terminal::Mode::Dec);
	for(u32 frame = 0; stk; ++frame, stk = (StackFrame *)stk->next) {
		PROMPT("Frame #", frame, " -> ", Terminal::Mode::HexOnce, stk->eip);
	}
}
