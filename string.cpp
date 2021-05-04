#include "string.h"

extern "C" {

siz strlen(const char *str) {
	siz len = 0;
	while(str[len]) len++;
	return len;
}

void memset(void *mem, u8 value, siz size) {
	// set in 32 bit chunks first
	u32 *dest32 = (u32 *)mem;
	u32 value32 = (u32)value << 24 | (u32)value << 16 | (u32)value << 8 | value;
	for(siz i = 0; i < size >> 2; i++) {
		*dest32++ = value32;
	}
	// finally, set the remaining bytes
	u8 *dest8 = (u8 *)dest32;
	for(siz i = 0; i < (size & 3); i++) {
		*dest8++ = value;
	}
}

void *memcpy(void *dest, const void *source, siz size) {
	// copy in 32 bit chunks first
	u32 *dest32   = (u32 *)dest;
	u32 *source32 = (u32 *)source;
	for(siz i = 0; i < size >> 2; i++) {
		*dest32++ = *source32++;
	}
	// finally, copy the remaining bytes
	u8 *dest8   = (u8 *)dest32;
	u8 *source8 = (u8 *)source32;
	for(siz i = 0; i < (size & 3); i++) {
		*dest8++ = *source8++;
	}
	return dest;
}

void *memmove(void *dest, const void *source, siz size) {
	u8 *dest8   = (u8 *)dest;
	u8 *source8 = (u8 *)source;
	// check if they overlap
	// it is not really a problem if dest starts before source,
	// in that case, memcpy can do usual sequential copy.
	// however, if source starts before dest, then that
	// becomes a problem, because all the bytes from 'dest'
	// to 'dest + (dest - source)' will be overwritten.
	// so, we first copy those bytes, then copy the rest of it.
	if(source8 < dest8 && dest8 <= source8 + size) {
		siz distance = dest8 - source8;
		// find out the overlapping bytes
		siz overlapping = size - distance;
		// copy the overlapping portion first
		memcpy(dest8 + distance, dest8, overlapping);
		// now copy 'distance' bytes from source to dest
		return memcpy(dest8, source8, distance);
	} else {
		// do plain memcpy
		return memcpy(dest, source, size);
	}
}

int memcmp(const void *source, const void *dest, siz size) {
	u32 *source64 = (u32 *)source;
	u32 *dest64   = (u32 *)dest;
	siz  i        = 0;
	siz  add      = size & 3;
	while(i++ < (size >> 2) && *source64++ == *dest64++)
		;

	if(i < (size >> 2)) {
		--source64;
		--dest64;
		add = 4;
	}

	u8 *source8 = (u8 *)source64;
	u8 *dest8   = (u8 *)dest64;
	u8 *end8    = source8 + add;
	while(source8 < end8 && *source8++ == *dest8++)
		;
	if(source8 == end8)
		return 0;
	--source8;
	--dest8;
	if(*source8 > *dest8) {
		return *source8 - *dest8;
	}
	return -(*dest8 - *source8);
}
}
