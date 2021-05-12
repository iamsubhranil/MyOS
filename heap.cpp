#include "heap.h"
#include "paging.h"
#include "stacktrace.h"
#include "terminal.h"

siz Heap::findSmallestHole(siz size, bool pageAlign) {
	siz i = 0;
	while(i < index.size) {
		Header *header = index.get(i);
		if(header->size >= size) {
			if(pageAlign) {
				uptr location = (uptr)header;
				uptr offset   = 0;
				if(!Paging::isAligned(location + sizeof(Header))) {
					offset = Paging::PageSize - ((location + sizeof(Header)) &
					                             (Paging::PageSize - 1));
				}
				if(header->size >= offset + size)
					break;
			} else
				break;
		}
		i++;
	}
	if(i == index.size)
		return -1;
	return i;
}

void Heap::init(uptr s, uptr e, siz m, bool supervisorOnly, bool readOnly) {
	index = OrderedArray<Header *>((void *)s, IndexSize, Header::lessThan);
	start = s;
	// move forward to the first available location,
	// rest are already occupied by the index.
	start += IndexSize;

	Paging::alignIfNeeded(start);

	end        = e;
	max        = m;
	properties = 0;
	properties |= supervisorOnly;
	properties |= (readOnly << 1);

	Header *hole = (Header *)start;
	hole->size   = end - start;
	hole->magic  = Header::Magic; // set is hole to true
	index.insert(hole);
}

void Heap::expand(siz newSize) {
	Paging::alignIfNeeded(newSize);

	siz oldSize = end - start;
	siz i       = oldSize;
	while(i < newSize) {
		Paging::getPage(start + i, 1, Paging::Directory::KernelDirectory)
		    ->alloc(isSupervisorOnly(), isReadOnly());
		i += Paging::PageSize;
	}
	end = start + newSize;
}

siz Heap::contract(siz newSize) {
	Paging::alignIfNeeded(newSize);

	if(newSize < Heap::MinHeapSize)
		newSize = Heap::MinHeapSize;

	siz oldSize = end - start;
	if(oldSize == newSize)
		return oldSize;
	// our last page actually starts from an index before
	siz i = oldSize - Paging::PageSize;
	while(newSize < i) {
		Paging::getPage(start + i, 0, Paging::Directory::KernelDirectory)
		    ->free();
		i -= Paging::PageSize;
	}
	end = start + newSize;
	return newSize;
}

void *Heap::alloc(siz size, bool pageAlign) {
	// take into account the size of header and footer
	siz newSize = size + sizeof(Header) + sizeof(Footer);

	siz iterator = findSmallestHole(newSize, pageAlign);

	if(iterator ==
	   (siz)-1) { // we didn't find a suitable hole, so we need to expand
		siz oldLength     = end - start;
		siz oldEndAddress = end;

		expand(oldLength + newSize);

		siz newLength = end - start;

		// Find the endmost header. (Not endmost in size, but in location)
		iterator = 0;
		siz idx = (u32)-1, value = 0;
		while(iterator < index.size) {
			uptr tmp = (uptr)index.get(iterator);
			if(tmp > value) {
				value = tmp;
				idx   = iterator;
			}
			iterator++;
		}

		if(idx == (u32)-1) {
			// no headers are found at all, so we need to add a new one
			Header *header = (Header *)oldEndAddress;
			header->magic  = Header::Magic; // is hole
			header->size   = newLength - oldLength;
			Footer *footer =
			    (Footer *)(oldEndAddress - header->size - sizeof(Footer));
			footer->magic  = Footer::Magic;
			footer->header = header;
			index.insert(header);
		} else {
			// adjust the last header
			Header *header = index.get(idx);
			header->size   = (newLength - oldLength);

			Footer *footer =
			    (Footer *)((uptr)header + header->size - sizeof(Footer));
			footer->header = header;
			footer->magic  = Footer::Magic;
		}
		// now recurse and allocate
		return alloc(size, pageAlign);
	}

	Header *origHoleHeader = index.get(iterator);
	uptr    origHolePos    = (uptr)origHoleHeader;
	siz     origHoleSize   = origHoleHeader->size;

	// Here we work out if we should split the hole we found into two parts.
	// Is the original hole size - requested hole size less than the overhead
	// for adding a new hole?
	if(origHoleSize - newSize < sizeof(Header) + sizeof(Footer)) {
		size += origHoleSize - newSize;
		newSize = origHoleSize;
	}

	if(pageAlign && !Paging::isAligned(origHolePos)) {
		uptr newLocation = origHolePos + Paging::PageSize -
		                   (origHolePos & (Paging::PageSize - 1)) -
		                   sizeof(Header);
		Header *holeHeader = (Header *)origHolePos;
		holeHeader->size   = Paging::PageSize -
		                   (origHolePos & (Paging::PageSize - 1)) -
		                   sizeof(Header);
		holeHeader->magic = Header::Magic; // mark it as a hole
		Footer *footer    = (Footer *)(newLocation - sizeof(Footer));
		footer->magic     = Footer::Magic;
		footer->header    = holeHeader;
		origHolePos       = newLocation;
		origHoleSize      = origHoleSize - holeHeader->size;
	} else {
		index.remove(iterator);
	}

	Header *header = (Header *)origHolePos;
	header->magic  = Header::Magic | 1; // mark as used
	header->size   = newSize;
	Footer *footer = (Footer *)(origHolePos + sizeof(Header) + size);
	footer->magic  = Footer::Magic;
	footer->header = header;

	if(origHoleSize > newSize) {
		Header *header =
		    (Header *)(origHolePos + sizeof(Header) + size + sizeof(Footer));
		header->magic = Header::Magic; // mark as a hole
		header->size  = origHoleSize - newSize;

		Footer *footer =
		    (Footer *)((uptr)header + origHoleSize - newSize - sizeof(Footer));

		if((uptr)footer < end) {
			footer->magic  = Footer::Magic;
			footer->header = header;
		}

		index.insert(header);
	}

	return (void *)((uptr)header + sizeof(Header));
}

void Heap::free(void *p) {
	if(!p)
		return;

	Header *header = (Header *)((uptr)p - sizeof(Header));
	Footer *footer = (Footer *)((uptr)header + header->size - sizeof(Footer));

	// last bit marks if it is a hole, so that should be set to 1 since it was
	// in use
	if((header->magic & ~((u32)1)) != Header::Magic) {
		Terminal::err("Invalid header magic!");
	}
	if(footer->magic != Footer::Magic) {
		Terminal::err("Invalid footer magic!");
	}
	header->magic = Header::Magic; // mark it as a hole
	// we may or may not want to add this header to free holes index, depending
	// on the left and right holes
	bool addToIndex = true;
	// try to unify left hole
	Footer *leftFooter = (Footer *)((uptr)header - sizeof(Footer));
	if(leftFooter->magic == Footer::Magic &&
	   leftFooter->header->magic == Header::Magic) {
		// we have a valid hole on the left
		// merge
		u32 bak        = header->size;
		header         = leftFooter->header;
		footer->header = header;
		header->size += bak;
		addToIndex = false;
	}

	// try to unify right hole
	Header *rightHeader = (Header *)((uptr)footer + sizeof(Footer));
	if(rightHeader->magic == Header::Magic) {
		// valid hole in right
		// merge
		header->size += rightHeader->size;
		footer =
		    (Footer *)((uptr)rightHeader + rightHeader->size - sizeof(Footer));
		footer->header = header; // point this footer to our header
		                         // remove right head
		u32 i = 0;
		while(i < index.size && index.get(i) != rightHeader) i++;
		// we should obviously find the item
		// now remove it.
		index.remove(i);
	}

	// try for contraction of the heap if this is the end address
	if((uptr)footer + sizeof(Footer) == end) {
		u32 oldLength = end - start;
		u32 newLength = contract((uptr)header - start);
		if(header->size > (oldLength - newLength)) {
			// this header should still exist, adjust the size
			header->size = oldLength - newLength;
			footer = (Footer *)((uptr)header + header->size - sizeof(Footer));
			footer->magic  = Footer::Magic;
			footer->header = header;
		} else {
			// this header does not exist anymore after contraction,
			// so remove it
			u32 i = 0;
			while(i < index.size && index.get(i) != header) i++;
			if(i < index.size) {
				addToIndex = false;
				index.remove(i);
			}
		}
	}

	if(addToIndex)
		index.insert(header);
}
