#include "string.h"

siz strlen(const char *str) {
	siz len = 0;
	while(str[len]) len++;
	return len;
}

void memset(void *mem, u8 value, u32 size) {
	u8 *ptr = (u8 *)mem;
	for(u32 i = 0; i < size; i++) {
		*ptr++ = value;
	}
}
