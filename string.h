#pragma once

#include "myos.h"

extern "C" {
extern siz   strlen(const char *str);
extern void  memset(void *mem, u8 value, siz size);
extern void *memcpy(void *dest, const void *source, siz size);
extern void *memmove(void *dest, const void *source, siz size);
extern int   memcmp(const void *s1, const void *s2, siz size);
// defined in memory.cpp
extern void *malloc(size_t size);
extern void  free(void *mem);
extern void  abort();
}
