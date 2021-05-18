#pragma once

#include "myos.h"

struct Register {
	uptr edi, esi, ebp, useless_esp, ebx, edx, ecx, eax; /* pushed by 'pusha' */
	uptr gs, fs, es, ds;                                 /* pushed explicitly */
	uptr int_no, err_code; /* our 'push byte #' and ecodes do this */
	uptr eip, cs, eflags;  /* pushed by the processor automatically */
	uptr esp, ss; /* these two are only pushed in case of a ring switch */

#define REG_DUMP2(r)                                                           \
	asm volatile("pusha\n"                                                     \
	             "pop %4\n"                                                    \
	             "pop %5\n"                                                    \
	             "pop %6\n"                                                    \
	             "pop %7\n"                                                    \
	             "pop %8\n"                                                    \
	             "pop %9\n"                                                    \
	             "pop %10\n"                                                   \
	             "pop %11\n"                                                   \
	             "push %%gs\n"                                                 \
	             "pop %0\n"                                                    \
	             "push %%fs\n"                                                 \
	             "pop %1\n"                                                    \
	             "push %%es\n"                                                 \
	             "pop %2\n"                                                    \
	             "push %%ds\n"                                                 \
	             "pop %3\n"                                                    \
	             "push %%cs\n"                                                 \
	             "pop %12\n"                                                   \
	             "push %%ss\n"                                                 \
	             "pop %13\n"                                                   \
	             "pushf\n"                                                     \
	             "pop %14\n"                                                   \
	             : "=g"((r)->gs), "=g"((r)->fs), "=g"((r)->es), "=g"((r)->ds), \
	               "=g"((r)->edi), "=g"((r)->esi), "=g"((r)->ebp),             \
	               "=g"((r)->esp), "=g"((r)->ebx), "=g"((r)->edx),             \
	               "=g"((r)->ecx), "=g"((r)->eax), "=g"((r)->cs),              \
	               "=g"((r)->ss), "=g"((r)->eflags)::"eax", "ebx", "ecx",      \
	               "edx", "edi", "esi");                                       \
	(r)->int_no = (r)->err_code = 0;                                           \
	(r)->eip                    = eip_load();
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
