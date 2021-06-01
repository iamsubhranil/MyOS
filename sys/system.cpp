#include <sys/system.h>
#include <drivers/terminal.h>

void Register::dump() const {
	Terminal::write(Terminal::Mode::Hex);
#define WRITE(x) Terminal::write(#x, ": ", x, ", ");
	WRITE(gs);
	WRITE(fs);
	WRITE(es);
	WRITE(ds);
	WRITE(edi);
	WRITE(esi);
	WRITE(ebp);
	WRITE(useless_esp);
	WRITE(ebx);
	WRITE(edx);
	WRITE(ecx);
	WRITE(eax);

	WRITE(int_no);
	WRITE(err_code);
	WRITE(eip);
	WRITE(cs);
	WRITE(eflags);
	WRITE(esp);
	WRITE(ss);

	Terminal::write("\n");
}
