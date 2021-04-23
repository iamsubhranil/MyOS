#pragma once

#include "myos.h"

struct Register {
	u32 gs, fs, es, ds;                         /* pushed the segs last */
	u32 edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pushed by 'pusha' */
	u32 int_no, err_code; /* our 'push byte #' and ecodes do this */
	u32 eip, cs, eflags, useresp,
	    ss; /* pushed by the processor automatically */
};
