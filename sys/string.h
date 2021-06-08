#pragma once

#include <sys/myos.h>

extern "C" {
extern siz strlen(const char *str);
// arch/x86/memset.S
// memset automatically copies in 4byte chunks if required
extern void memset(void *mem, u8 value, siz size);
// arch/x86/memcpy.S
// copies in 4byte chunks if required
extern void *memcpy(void *dest, const void *source, siz size);
// arch/x86/memcmp.S
// automatically compares in 4byte chunks if required
extern int memcmp(const void *s1, const void *s2, siz size);

extern void *memmove(void *dest, const void *source, siz size);

// defined in memory.cpp
extern void *malloc(size_t size);
extern void  free(void *mem);
extern void  abort();
}
