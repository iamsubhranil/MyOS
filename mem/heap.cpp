#include <drivers/terminal.h>
#include <mem/heap.h>
#include <mem/paging.h>
#include <sched/scopedlock.h>
#include <sys/string.h>

void Heap::init(siz size) {
	// size must be pagealigned
	Paging::alignIfNeeded(size);
	heapStart = (uptr)this;
	heapEnd   = heapStart + size;
	// after us, the first we'll do is make space for
	// all the buckets that we may create in the future.
	buckets = (Bucket *)(heapStart + sizeof(Heap));

	// we'll use the first page for ourselves, and leave
	// the rest of the place for everything else.
	uptr usable = size - Paging::PageSize;
	// now find the number of buckets we can fit
	maxBucketMemory     = usable / BucketRatio;
	numBuckets          = maxBucketMemory / BucketSize;
	bucketAdditionalMem = numBuckets * sizeof(Bucket);
	// is the total overhead required more than our allocated
	// 4KiB?
	if(bucketAdditionalMem + sizeof(Heap) > Paging::PageSize) {
		// try to find an optimal range where we will have
		// enough place to place the buckets themselves
		while(bucketAdditionalMem + sizeof(Heap) + usable > size) {
			// reduce the usable memory by a page
			usable -= Paging::PageSize;
			maxBucketMemory     = usable / BucketRatio;
			numBuckets          = maxBucketMemory / BucketSize;
			bucketAdditionalMem = numBuckets * sizeof(Bucket);
		}
	}
	// make sure all the pages needed by us to manage the heap
	// are allocated
	for(uptr i = (uptr)heapStart; i < (uptr)buckets + bucketAdditionalMem;
	    i += Paging::PageSize) {
		Paging::getPage(i, true, Paging::Directory::CurrentDirectory)
		    ->alloc(true, true);
	}
	// we'll use 'usable' amount of memory from the end of this heap,
	// so calculate that first.
	uptr start = (uptr)(heapEnd - usable);
	// page align the starting address
	Paging::alignIfNeeded(start);
	bucketAllocationCurrent = bucketAllocationStart = start;
	bucketAllocationEnd  = bucketAllocationStart + maxBucketMemory - 1;
	largeAllocationStart = bucketAllocationEnd + 1;
	largeAllocationEnd   = heapEnd;

	for(siz i = 0; i < BlockCount; i++) bucketClassList[i] = NULL;

	freeBuckets = NULL;

	// initialize the large header
	headerRoot = (Header *)largeAllocationStart;
	headerRoot->ensureMapped();
	headerRoot->allocationSize = (largeAllocationEnd - largeAllocationStart);
	headerRoot->left           = NULL;
	headerRoot->right          = NULL;
	headerRoot->previousHeader = NULL;
	headerRoot->magic          = Header::Magic;

	heapLock = SpinLock();
}

void *Heap::alloc(siz bytes) {
	ScopedLock sl(heapLock); // make sure only one thread accesses it
	if(bytes <= BlockEnd) {
		// we can do bucket allocation
		bytes = blockNearest(bytes);
		siz c = getSizeClass(bytes);
		if(bucketClassList[c]) {
			Bucket *b = bucketClassList[c];
			void *  m = b->allocateBlock();
			if(m)
				return m;
			// try to find a bucket which can allocate this block
			for(Bucket *buk = b->nextBucket; buk != NULL;
			    b = buk, buk = buk->nextBucket) {
				void *m = buk->allocateBlock();
				if(m) {
					// this bucket did find something to allocate
					// put this bucket to the front of the class list
					b->nextBucket      = buk->nextBucket;
					buk->nextBucket    = bucketClassList[c];
					bucketClassList[c] = buk;
					return m;
				}
			}
		}
		// try to allocate a new bucket
		Bucket *b = allocBucket(bytes, c);
		if(!b)
			return NULL;
		return b->allocateBlock();
	} else {
		// round up to the next multiple of 8 to make sure
		// our allocation stays aligned
		bytes = roundUp8(bytes);
		// use huge allocators
		Header *h = findClosestHeader(bytes);
		if(!h) {
			Terminal::err("No free header found to allocate!\n");
			for(;;)
				;
		}
		h->magic = Header::Magic | 1;
		// remove it from the tree
		removeHeader(h);
		// check if we can break it
		// we'll only break a header if it contains enough space to allocate
		// a new huge object, i.e. of size BlockEnd + 1
		if(h->allocationSize > bytes + sizeof(Header) + BlockEnd) {
			Header *nh = (Header *)((uptr)h + sizeof(Header) + bytes);
			nh->ensureMapped();
			nh->magic          = Header::Magic;
			nh->left           = NULL;
			nh->right          = NULL;
			nh->previousHeader = h;
			nh->allocationSize = h->allocationSize - bytes - sizeof(Header);
			// insert the new header
			insertHeader(nh);
			// adjust the old header
			h->allocationSize = bytes + sizeof(Header);
		}
		h->ensureMapped(true);
		return (void *)((uptr)h + sizeof(Header));
	}
}

void *Heap::alloc_a(siz bytes) {
	ScopedLock sl(heapLock); // make sure only one thread accesses it
	if(bytes <= BlockEnd) {
		bytes   = blockNearest(bytes);
		siz cls = getSizeClass(bytes);
		// to alloc a page aligned byte within a bucket,
		// it must be the first allocation of a bucket,
		// buckets themselves are page aligned. so, find
		// a free bucket.
		Bucket *b = allocBucket(bytes, cls);
		if(!b) {
			Terminal::err("No free bucket found to allocate aligned!\n");
			for(;;)
				;
		}
		return b->allocateBlock();
	} else {
		bytes = roundUp8(bytes);
		// try to find a free header
		Header *header = findClosestHeader(bytes, true);
		if(!header) {
			Terminal::err("No free header found to allocate aligned!\n");
			for(;;)
				;
		}
		// remove ourselves from the tree first
		removeHeader(header);
		// mark it used
		header->magic |= 1;
		uptr addrStart = (uptr)header + sizeof(Header);
		// make the address aligned
		Paging::alignIfNeeded(addrStart);
		if(addrStart - sizeof(Header) == (uptr)header) {
			// we are luckily in the perfect position to make
			// the returned memory page aligned, so we don't
			// need to do anything except for following
			// the routine procedure.
		} else {
			// we'll start from addrStart - sizeof(Header).
			uptr newStart = addrStart - sizeof(Header);
			// populate the new header
			Header *newHeader = (Header *)newStart;
			newHeader->ensureMapped();
			newHeader->magic = Header::Magic | 1;
			newHeader->left = newHeader->right = NULL;
			// this is the additional amount of memory that
			// we will release
			uptr additionalSize = newStart - (uptr)header;
			// so, this will be our new size
			newHeader->allocationSize = header->allocationSize - additionalSize;
			// try to adjust our previous header
			Header *prev              = header->previousHeader;
			newHeader->previousHeader = prev;
			// if we don't even have a previous header, we need
			// to create a new one, providing we have enough
			// space to create a new header. otherwise, we'll
			// just keep the beginning of the allocation block
			// empty, free will adjust us back.
			if(!prev) {
				if(additionalSize > sizeof(Header) + BlockEnd) {
					Header *add = header; // this is our new fragmented header
					add->allocationSize = additionalSize;
					add->previousHeader = NULL;
					add->left = add->right    = NULL;
					add->magic                = Header::Magic;
					newHeader->previousHeader = add;
					insertHeader(add);
				} else {
					// we don't have enough size in front of us
					// to perform a huge allocation. so keep the space empty
				}
			} else {
				// we have a previous header, remove that from the tree
				// if it was free
				if(prev->magic == Header::Magic)
					removeHeader(prev);
				// adjust its size
				prev->allocationSize += additionalSize;
				// insert that back
				if(prev->magic == Header::Magic)
					insertHeader(prev);
			}
			header = newHeader;
		}
		// finally, check if we have anough space to break us up
		// we'll only break a header if it contains enough space to allocate
		// a new huge object, i.e. of size BlockEnd + 1
		if(header->allocationSize > bytes + sizeof(Header) + BlockEnd) {
			Header *nh = (Header *)((uptr)header + sizeof(Header) + bytes);
			nh->ensureMapped();
			nh->magic          = Header::Magic;
			nh->left           = NULL;
			nh->right          = NULL;
			nh->previousHeader = header;
			nh->allocationSize =
			    header->allocationSize - bytes - sizeof(Header);
			// insert the new header
			insertHeader(nh);
			// adjust the old header
			header->allocationSize = bytes + sizeof(Header);
		}
		header->ensureMapped(true);
		return (void *)((uptr)header + sizeof(Header));
	}
}

void Heap::free(void *mem) {
	ScopedLock sl(heapLock); // make sure only one thread accesses it
	if(!mem)
		return;
	uptr addr = (uptr)mem;
	if(addr < bucketAllocationEnd) {
		// this is allocated by a bucket, so find it
		siz     idx = getBucketIndex(addr);
		Bucket *b   = &buckets[idx];
		b->releaseBlock(mem);
		siz cls = getSizeClass(b->blockSize);
		if(b->numAvailBlocks == BucketSize / b->blockSize) {
			// add this bucket to the free list
			Bucket *parent = NULL;
			Bucket *c      = NULL;
			for(c = bucketClassList[cls]; c != b; parent = c, c = c->nextBucket)
				;
			if(parent)
				parent->nextBucket = b->nextBucket;
			else
				bucketClassList[cls] = b->nextBucket;
			b->nextBucket = freeBuckets;
			freeBuckets   = b;
			// release the page back to the os
			// we can only do this because we know
			// startMem is page aligned and has size
			// equal to the page size
			Paging::getPage((uptr)b->startMem, false,
			                Paging::Directory::CurrentDirectory)
			    ->free();
		}
		// otherwise, we may also want to add this bucket to the front
		// of its size class, but that may generate unnecessary additional
		// overhead. we'll see.
	} else {
		// find the header
		Header *h = (Header *)((uptr)mem - sizeof(Header));
		if(h->magic != (Header::Magic | 1)) {
			Terminal::err("Invalid header magic!\n");
			for(;;)
				;
		}
		// mark us as free
		h->magic = Header::Magic;
		// check if next header is free, iff we're not the last header
		Header *nh = (Header *)((uptr)h + h->allocationSize);
		if((uptr)nh != largeAllocationEnd && nh->magic == Header::Magic) {
			// merge with next header, and remove next
			Header *nnh = (Header *)((uptr)nh + nh->allocationSize);
			if((uptr)nnh != largeAllocationEnd)
				nnh->previousHeader = h;
			// remove next header
			removeHeader(nh);
			// add its allocation size to ours
			h->allocationSize += nh->allocationSize;
		}
		// check if its previous header is free, iff there is one
		if(h->previousHeader && h->previousHeader->magic == Header::Magic) {
			// remove previous header
			removeHeader(h->previousHeader);
			// merge with previous header
			h->previousHeader->allocationSize += h->allocationSize;
			// adjust the pointer
			Header *nh = (Header *)((uptr)h + h->allocationSize);
			if((uptr)nh != largeAllocationEnd)
				nh->previousHeader = h->previousHeader;
			h = h->previousHeader;
		}
		// if we don't have a previous header but we're still not
		// at where we should be, that means some space is leftover
		// in the beginning from a previous aligned allocation
		if(h->previousHeader == NULL && (uptr)h != largeAllocationStart) {
			// create a new header
			Header *newHeader = (Header *)largeAllocationStart;
			newHeader->allocationSize =
			    h->allocationSize + ((uptr)h - largeAllocationStart);
			newHeader->magic          = Header::Magic;
			newHeader->previousHeader = NULL;
			newHeader->left = newHeader->right = NULL;
			// search for the next header
			Header *nh = (Header *)(newHeader + newHeader->allocationSize);
			if((uptr)nh < largeAllocationEnd) {
				// adjust its previous header
				nh->previousHeader = newHeader;
			}
			h = newHeader;
		}
		// finally, insert us back
		insertHeader(h);
	}
}

Heap::Bucket *Heap::allocBucket(siz size, siz cls) {
	// try to check if we have a free bucket
	Bucket *b = NULL;
	if(freeBuckets) {
		b           = freeBuckets;
		freeBuckets = freeBuckets->nextBucket;
		// map the page
		Paging::getPage((uptr)b->startMem, false,
		                Paging::Directory::CurrentDirectory)
		    ->alloc(true, true);
		b->init(size);
	} else {
		if(bucketAllocationCurrent > bucketAllocationEnd) {
			Terminal::err("No more memory to allocate a bucket!\n");
			for(;;)
				;
		}
		// try to allocate a new bucket
		siz idx = getBucketIndex(bucketAllocationCurrent);
		b       = &buckets[idx];
		b       = Bucket::create(size, (uptr)b, bucketAllocationCurrent);
		// if the page is not yet allocated, alloc it
		Paging::getPage((uptr)bucketAllocationCurrent, true,
		                Paging::Directory::CurrentDirectory)
		    ->alloc(true, true);
		bucketAllocationCurrent += BucketSize;
	}
	if(b == NULL)
		return NULL;
	b->nextBucket        = bucketClassList[cls];
	bucketClassList[cls] = b;
	return b;
}

Heap::Header *Heap::findClosestHeader(siz size, bool pageAlign) {
	Header *lastFound = NULL;
	size += sizeof(Header);
	Header *search = headerRoot;
	while(search) {
		if(search->allocationSize >= size && search->magic == Header::Magic) {
			if(pageAlign) {
				// check if the header is page aligned
				// check if the size criteria will still satisfy
				// if we add blank space up until the next page
				uptr bak = (uptr)search;
				Paging::alignIfNeeded(bak);
				// so, our target memory will start from bak, header will
				// start from bak - sizeof(Header), and the whole
				// thing will end at bak + size - sizeof(Header)
				uptr end = bak + size - sizeof(Header);
				// so we need at least this amount of memory
				uptr total = end - (uptr)search;
				// check if we have it
				if(search->allocationSize >= total) {
					// we do, fine
					lastFound = search;
				}
			} else
				lastFound = search;
		}
		if(search->allocationSize >= size) {
			search = search->left;
		} else if(search->allocationSize < size) {
			search = search->right;
		}
	}
	return lastFound;
}

void Heap::insertHeader(Header *h) {
	Header **pos = &headerRoot;
	while(*pos) {
		if((*pos)->allocationSize >= h->allocationSize) {
			pos = &(*pos)->left;
		} else {
			pos = &(*pos)->right;
		}
	}
	*pos = h;
}

void Heap::removeHeader(Header *h, Header **slot) {
	if(slot == NULL) {
		slot = &headerRoot;
		while(*slot != h) {
			if(h->allocationSize > (*slot)->allocationSize)
				slot = &(*slot)->right;
			else
				slot = &(*slot)->left;
		}
	}
	if(h->left && h->right) {
		// find the inorder successor
		Header **childSlot    = &h->right;
		Header * inorder_succ = h->right;
		while(inorder_succ->left) {
			childSlot    = &inorder_succ->left;
			inorder_succ = inorder_succ->left;
		}
		// remove the successor from where it was
		removeHeader(inorder_succ, childSlot);
		// assign it here
		*slot = inorder_succ;
		// adjust the children
		inorder_succ->left  = h->left;
		inorder_succ->right = h->right;
	} else if(h->left || h->right) {
		Header *node = h->left == NULL ? h->right : h->left;
		*slot        = node;
	} else {
		*slot = NULL;
	}
	// finally, clear the left right pointers of the header
	h->left = h->right = NULL;
}

void Heap::Header::ensureMapped(bool full) {
	// map the first page
	Paging::getPage((uptr)this, true, Paging::Directory::CurrentDirectory)
	    ->alloc(true, true);
	if(full) {
		for(uptr i = (uptr)this + Paging::PageSize;
		    i < (uptr)this + allocationSize; i += Paging::PageSize)
			Paging::getPage(i, true, Paging::Directory::CurrentDirectory)
			    ->alloc(true, true);
	}
}
