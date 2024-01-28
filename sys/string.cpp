#include <drivers/terminal.h>
#include <mem/memory.h>
#include <sys/stacktrace.h>
#include <sys/string.h>

extern "C" {

char *strdup(const char *str) {
	siz   len = strlen(str);
	char *s   = (char *)Memory::alloc(sizeof(char) * (len + 1));
	memcpy(s, str, sizeof(char) * len);
	s[len] = 0;
	return s;
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

int strcmp(const char *s1, const char *s2) {
	while(*s1 && *s2 && *s1 == *s2) {
		s1++;
		s2++;
	}
	if(*s1 != 0 && *s2 != 0) {
		return *s1 - *s2;
	} else if(*s1 != 0) {
		return 1;
	} else if(*s2 != 0) {
		return -1;
	} else {
		return 0;
	}
}

void abort() {
	Terminal::spinlock.unlock();
	Terminal::err("abort() called!");
	Stacktrace::print();
}
}

u32 StringSlice::dump() const {
	for(int i = start; i < end; i++) Terminal::write(s[i]);
	return end - start;
}
