#pragma once

// we are going for a higher half kernel
#define KMEM_BASE 0xC0000000
#define KMEM_GRUB 0x00100000

#define KMEM_LINK (KMEM_BASE + KMEM_GRUB)

// Linear frame buffer reserve
#define FB_ADDR_RESERVE 0xFF800000

#define PAGE_TABLES_RESERVE 0xFFC00000
#define PAGE_DIR_RESERVE 0xFFFFF000

#define KHEAP_END FB_ADDR_RESERVE

#define V2P(x) (((uptr)(x)-KMEM_BASE))
#define P2V(x) ((void *)((uptr)(x) + KMEM_BASE))

#define V2P_WO(x) ((x)-KMEM_BASE)
#define P2V_WO(x) ((x) + KMEM_BASE)

#define PDE_PRESENT 0x001
#define PDE_WRITABLE 0x002
#define PDE_USER 0x004
#define PDE_WRITE_THROUGH 0x008
#define PDE_CACHE_DISABLE 0x010
#define PDE_ACCESSED 0x020
#define PDE_PAGE_SIZE 0x080
#define PDE_GLOBAL 0x100
#define PDE_RESERVED 0x040

#define PTE_PRESENT 0x001
#define PTE_WRITABLE 0x002
#define PTE_USER 0x004
#define PTE_WRITE_THROUGH 0x008
#define PTE_CACHE_DISABLE 0x010
#define PTE_ACCESSED 0x020
#define PTE_DIRTY 0x040
#define PTE_PAT_BIT_3 0x080
#define PTE_GLOBAL 0x100
