#pragma once

#include "myos.h"

extern "C" {
siz   strlen(const char *str);
void  memset(void *mem, u8 value, siz size);
void *memcpy(void *dest, const void *source, siz size);
void *memmove(void *dest, const void *source, siz size);
int   memcmp(const void *s1, const void *s2, siz size);
}
