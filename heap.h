#pragma once

#include "myos.h"
#include "option.h"
#include "ordered_array.h"

struct Heap {
	static const uptr Start = 0xD0000000;

	// block header
	struct Header {
		static const u32 Magic = 0x48454144; // HEAD

		u32 magic; // if it is a hole, last bit is set to 0, if it is set to 1,
		           // it is in use
		siz size;  // including footer

		bool isHole() {
			return magic == Magic;
		}

		static bool lessThan(Header *const &a, Header *const &b) {
			return a->size < b->size;
		}
	};

	static const siz IndexCount  = 0x20000;
	static const siz IndexSize   = sizeof(Header) * IndexCount;
	static const siz MinHeapSize = 0x400000;
	static const siz MaxHeapSize = 0xFFFF000; // 256 MiB
	static const siz InitialSize = IndexSize + MinHeapSize;

	// block footer
	struct Footer {
		static const u32 Magic = 0x464f4f54; // FOOT

		u32     magic;
		Header *header; // pointer to the header of this block
	};

	// members of the heap

	// collection of holes
	OrderedArray<Header *> index;
	// address range
	uptr start, end;
	siz  max;
	// first bit denotes whether the extra pages requested by us should be
	// mapped as supervisor-only.
	// second bit denotes whether extra pages should be mapped as read-only
	u8 properties;

	bool isSupervisorOnly() {
		return properties & 1;
	}

	bool isReadOnly() {
		return properties & 2;
	}

	Heap(uptr start, uptr end, siz max, bool supervisorOnly, bool readOnly) {
		init(start, end, max, supervisorOnly, readOnly);
	}
	void  init(uptr start, uptr end, siz max, bool supervisorOnly,
	           bool readOnly);
	void *alloc(siz size, bool pageAlign);
	void  free(void *p);

	Option<siz> findSmallestHole(siz size, bool pageAlign);
	void        expand(siz newSize);
	siz         contract(siz newSize);
};
