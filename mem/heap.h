#pragma once

#include <mem/paging.h>
#include <sched/spinlock.h>
#include <sys/myos.h>

struct Heap {

	// we maintain caches from 0 - blockEnd bytes, with a blockWidth
	// byte window for each list. i.e., a request between 0 - blockWidth
	// bytes always gets blockWidth bytes allocated for it.
	static const siz BlockEnd            = 1024;
	static const siz BlockWidth          = 8; // MUST BE A POWER OF 2
	static const siz BlockWidthBitNumber = 3;
	static const siz BlockCount          = BlockEnd / BlockWidth;
	// finds the class for a size
	constexpr static siz getSizeClass(const siz size) {
		return (size >> BlockWidthBitNumber) - 1;
	}
	// finds the nearest block
	constexpr static siz blockNearest(const siz value) {
		return (value + BlockWidth - 1) & -BlockWidth;
	}

	struct Block {
		// it holds the address of the next block in memory.
		// if this is nullptr, it denotes that the next block
		// is not carved out yet, and is at blockSize/poolSize
		// offset from the present block, as applicable.
		void *nextBlock;
	};

	static const siz BucketSize = 4 * 1024; // 4 KiB
	// Buckets are allocator for homogeneous sizes
	struct Bucket {
		// start and end of this bucket's memory
		uptr startMem;
		uptr endMem;
		// number of available blocks in this bucket
		siz numAvailBlocks;
		// pointer to the next free block in this bucket
		Block *nextBlock;
		// last allocated block in this bucket
		uptr lastBlock;
		// size of the blocks in this bucket
		siz blockSize;
		// pointer to the next bucket in
		// the same arena of the same
		// size class
		struct Bucket *nextBucket;
		// will return NULL if the bucket
		// does not have any more block
		// to allocate
		void *allocateBlock() {
			if(numAvailBlocks > 0) {
				numAvailBlocks--;
				if(nextBlock == nullptr) {
					return (void *)(lastBlock += blockSize);
				} else {
					void *ret = nextBlock;
					nextBlock = (Block *)nextBlock->nextBlock;
					return ret;
				}
			}
			return nullptr;
		}

		void releaseBlock(void *m) {
			Block *b     = (Block *)m;
			b->nextBlock = nextBlock;
			nextBlock    = b;
			numAvailBlocks++;
		}

		static Bucket *create(siz blockSize, uptr bucketMem, uptr mem) {
			Bucket *p = (Bucket *)bucketMem;
			// initialize the memory boundary one time
			p->startMem = mem;
			p->endMem   = (uptr)mem + BucketSize - 1;
			p->init(blockSize);
			return p;
		}

		void init(siz blockSiz) {
			lastBlock      = ((uptr)startMem - blockSiz);
			blockSize      = blockSiz;
			numAvailBlocks = BucketSize / blockSiz;
			nextBucket     = nullptr;
			nextBlock      = nullptr;
		}
	};

	static const siz KHeapStart = 0xD0000000;
	static const siz KHeapEnd   = 0xDFFFFFFF;

	// the page directory associated with this heap
	Paging::Directory *directory;

	SpinLock
	    heapLock; // make sure only one thread accesses alloc/free at a time

	uptr heapStart; // start of the heap
	uptr heapEnd;   // end of the heap
	// this is the ratio of memory shared between buckets
	// and large allocations. buckets get total / BucketRatio
	// amount of memory. rest is given to large allocs.
	static const siz BucketRatio = 2;
	// Among the total heap, this denotes how much memory
	// can be used for buckets
	siz maxBucketMemory;
	siz numBuckets; // number of buckets in this heap
	// additional amount of memory required for managing the bucket structures
	siz bucketAdditionalMem;
	// array of buckets.
	// chunk at an address will belong to one of the buckets
	// in this array by following some linear calculations
	Bucket *buckets;
	// Linked list of buckets sorted by size class
	Bucket *bucketClassList[BlockCount];
	Bucket *freeBuckets; // linked list of free buckets
	// all of the pointers below must be aligned on page boundaries

	// this is the place from where chunk of memory is
	// given out to the buckets.
	uptr bucketAllocationStart;
	// currently from where a new bucket should
	// be allocated
	uptr bucketAllocationCurrent;
	// end of the memory space reserved for bucket
	// allocations
	uptr bucketAllocationEnd;

	siz getBucketIndex(uptr address) {
		return (address - bucketAllocationStart) >>
		       12; // log2(size of each bucket)
	}

	Bucket *allocBucket(siz blockSize, siz cls);
	// beginning of large memory allocation
	uptr largeAllocationStart;
	// end of the same
	uptr largeAllocationEnd;

	// structures to manage huge memory allocations.
	// the free headers form a binary tree. as well
	// as, they keep track of their neighbors.
	// nextHeader = (uptr)header + allocationSize;
	struct Header {
		static const u32 Magic = 0x48454144; // HEAD

		u32     magic;
		siz     allocationSize; // including the header
		Header *previousHeader; // previous header in memory

		Header *left;
		Header *right;

		// ensures that the page this header belongs is
		// mapped already. If full is true, this ensures
		// that all the pages upto allocationSize is mapped
		void ensureMapped(Paging::Directory *directory, bool full = false);
	};

	// only free headers are kept in the tree.
	Header *headerRoot;
	Header *findClosestHeader(siz size, bool pageAlign = false);
	void    insertHeader(Header *h);
	void    removeHeader(Header *h, Header **slot = NULL);
	// round up to next multiple of 8
	static constexpr siz roundUp8(siz value) {
		return ((value + 7) & -8);
	}

	// main allocation functions
	void *alloc(siz size);
	void *alloc_a(siz size);
	void  free(void *mem);

	// base contains the base address of start of the heap
	// size contains the total size of the heap. the heap will
	// not allocate all the pages upfront, it will just reserve
	// them.
	void init(siz base, siz size, Paging::Directory *dir);
};
