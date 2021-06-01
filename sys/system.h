#pragma once

#include <sys/myos.h>

struct Register {
	uptr edi, esi, ebp, useless_esp, ebx, edx, ecx, eax; /* pushed by 'pusha' */
	uptr gs, fs, es, ds;                                 /* pushed explicitly */
	uptr int_no, err_code; /* our 'push byte #' and ecodes do this */
	uptr eip, cs, eflags;  /* pushed by the processor automatically */
	uptr esp, ss; /* these two are only pushed in case of a ring switch */

#define REG_DUMP2(x)                                                      \
	Register *r = (Register *)(x);                                        \
	asm("pusha\n"                                                         \
	    "push %%cs\n"                                                     \
	    "push %%ds\n"                                                     \
	    "push %%es\n"                                                     \
	    "push %%fs\n"                                                     \
	    "push %%gs\n"                                                     \
	    "pushf\n"                                                         \
	    "pop %0\n"                                                        \
	    "pop %1\n"                                                        \
	    "pop %2\n"                                                        \
	    "pop %3\n"                                                        \
	    "pop %4\n"                                                        \
	    "pop %5\n"                                                        \
	    "pop %6\n"                                                        \
	    "pop %7\n"                                                        \
	    "pop %8\n"                                                        \
	    "pop %9\n"                                                        \
	    "pop %10\n"                                                       \
	    "pop %11\n"                                                       \
	    "pop %12\n"                                                       \
	    "pop %13\n"                                                       \
	    : "=g"((r)->eflags), "=g"((r)->gs), "=g"((r)->fs), "=g"((r)->es), \
	      "=g"((r)->ds), "=g"((r)->cs), "=g"((r)->edi), "=g"((r)->esi),   \
	      "=g"((r)->ebp), "=g"((r)->useless_esp), "=g"((r)->ebx),         \
	      "=g"((r)->edx), "=g"((r)->ecx), "=g"((r)->eax));                \
	(r)->int_no = (r)->err_code = 0;                                      \
	(r)->esp = (r)->ss = 0;                                               \
	(r)->eip           = eip_load();
	// DOES NOT COPY SS AND ESP
	Register &operator=(const Register &r) {
		gs = r.gs;
		fs = r.fs;
		es = r.es;
		ds = r.ds;

		edi         = r.edi;
		esi         = r.esi;
		ebp         = r.ebp;
		useless_esp = r.useless_esp;
		ebx         = r.ebx;
		edx         = r.edx;
		ecx         = r.ecx;
		eax         = r.eax;

		int_no   = r.int_no;
		err_code = r.err_code;

		eip    = r.eip;
		cs     = r.cs;
		eflags = r.eflags;
		return *this;
	}
	void dump() const;
};
