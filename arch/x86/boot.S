#include <arch/x86/kernel_layout.h>

/* Declare constants for the multiboot header. */
.set ALIGN,     1<<0            /* align loaded modules on page boundaries */
.set MEMINFO,   1<<1            /* provide memory map */
.set VIDEO,     1<<2            /* provide info about video mode */
.set FLAGS,    ALIGN | MEMINFO | VIDEO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* 
Declare a multiboot header that marks the program as a kernel. These are magic
values that are documented in the multiboot standard. The bootloader will
search for this signature in the first 8 KiB of the kernel file, aligned at a
32-bit boundary. The signature is in its own section so the header can be
forced to be within the first 8 KiB of the kernel file.
*/
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.skip 24
.long 0 // linear framebuffer
.long 1024 // width
.long 768 // height
.long 0 // bpp

/*
The multiboot standard does not define the value of the stack pointer register
(esp) and it is up to the kernel to provide a stack. This allocates room for a
small stack by creating a symbol at the bottom of it, then allocating 16384
bytes for it, and finally creating a symbol at the top. The stack grows
downwards on x86. The stack is in its own section so it can be marked nobits,
which means the kernel file is smaller because it does not contain an
uninitialized stack. The stack on x86 must be 16-byte aligned according to the
System V ABI standard and de-facto extensions. The compiler will assume the
stack is properly aligned and failure to align the stack will result in
undefined behavior.
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

// Preallocate pages used for paging. Don't hard-code addresses and assume they
// are available, as the bootloader might have loaded its multiboot structures or
// modules there. This lets the bootloader know it must avoid the addresses.
.section .data, "aw"
    .align 4096
.global boot_page_directory
boot_page_directory:
    .fill 1024, 4, 0
boot_page_table1:
    .fill 1024, 4, 0

.section .text
.global _start
.type _start, @function
_start:

    // first, let's set up a page table for our initial loader
    // Physical address of boot_page_table1.
    movl $V2P_WO(boot_page_table1), %edi
    movl $0, %esi
    // Map 1024 pages
    movl $1024, %ecx

.page_table_loop:
    // Map physical address as "present, writable". Note that this maps
    // .text and .rodata as writable. Mind security and map them as non-writable.
    movl %esi, %edx
    orl $(PTE_PRESENT | PTE_WRITABLE | PTE_USER), %edx
    movl %edx, (%edi)

    // Size of page is 4096 bytes.
    addl $4096, %esi
    // Size of entries in boot_page_table1 is 4 bytes.
    addl $4, %edi

    // Map until the end of the kernel (OR MAP ALL 1024 PAGES!!)
    // cmpl $V2P_WO(__ld_kernel_end), %esi
    // jge 3f

    // Loop to the next entry if we haven't finished.
    // this will continue until ecx = 0
    loop .page_table_loop
3:
    // Map the page table to both virtual addresses 0x00000000 and 0xC0000000.
    movl $V2P_WO(boot_page_table1), %ecx
    orl $(PDE_PRESENT | PDE_WRITABLE | PDE_USER), %ecx

    movl %ecx, V2P_WO(boot_page_directory) + 0
    movl %ecx, V2P_WO(boot_page_directory) + 0xc00

    // Set cr3 to the address of the boot_page_directory.
    movl $V2P_WO(boot_page_directory), %ecx
    movl %ecx, %cr3

    // Enable paging and the write-protect bit.
    movl %cr0, %ecx
    orl $0x80000001, %ecx
    movl %ecx, %cr0

    // Jump to higher half with an absolute jump.
    lea 1f, %ecx
    jmp *%ecx

1:
    // At this point, paging is fully set up and enabled.
    // Unmap the identity mapping as it is now unnecessary.
    movl $0, boot_page_directory + 0

    // Reload crc3 to force a TLB flush so the changes to take effect.
    // movl %cr3, %ecx
    // movl %ecx, %cr3

    // invalidate page at 0x00000000
    invlpg (0)

    // Set up the stack.
    movl $stack_top, %esp

    push $0
    push $0
    push $stack_top
    // push multiboot headers
    addl $KMEM_BASE, %ebx // make it a virtual address
    push %ebx   // multiboot header pointer
    xor %ebp, %ebp // set ebp to 0 so we know where to stop which stacktracking
	call kernelMain

	/*
	If the system has nothing more to do, put the computer into an
	infinite loop. To do that:
	1) Disable interrupts with cli (clear interrupt enable in eflags).
	   They are already disabled by the bootloader, so this is not needed.
	   Mind that you might later enable interrupts and return from
	   kernel_main (which is sort of nonsensical to do).
	2) Wait for the next interrupt to arrive with hlt (halt instruction).
	   Since they are disabled, this will lock up the computer.
	3) Jump to the hlt instruction if it ever wakes up due to a
	   non-maskable interrupt occurring or due to system management mode.
	*/
	cli
1:	hlt
	jmp 1b

.global gdtFlush

gdtFlush:
    lgdt  (__gdtptr)
   /* Reload data segment registers */
   mov   $0x10, %ax /* 0x10 points at the new data selector */
   mov   %ax, %ds
   mov   %ax, %es
   mov   %ax, %fs
   mov   %ax, %gs
   mov   %ax, %ss
   /* Reload CS register containing code selector */
   jmp   $0x08, $reload_CS /* 0x08 points at the new code selector */

reload_CS:
   ret

.global tssFlush

tssFlush:
   mov $0x2B, %ax      /* Load the index of our TSS structure - The index is
                     ; 0x28, as it is the 5th selector and each is 8 bytes
                     ; long, but we set the bottom two bits (making 0x2B)
                     ; so that it has an RPL of 3, not zero. */
   ltr %ax            /* Load 0x2B into the task state register. */
   ret

/* IDTs */
/* Division By Zero Exception */
.global _isr0
_isr0:
	cli
	push $0
	push $0
	jmp isr_common_stub

/* Debug Exception */
.global _isr1
_isr1:
	cli
	push $0
	push $1
	jmp isr_common_stub

/* Non Maskable Interrupt Exception */
.global _isr2
_isr2:
	cli
	push $0
	push $2
	jmp isr_common_stub

/* Breakpoint Exception */
.global _isr3
_isr3:
	cli
	push $0
	push $3
	jmp isr_common_stub

/* Into Detected Overflow Exception */
.global _isr4
_isr4:
	cli
	push $0
	push $4
	jmp isr_common_stub

/* Out of Bounds Exception */
.global _isr5
_isr5:
	cli
	push $0
	push $5
	jmp isr_common_stub

/* Invalid Opcode Exception */
.global _isr6
_isr6:
	cli
	push $0
	push $6
	jmp isr_common_stub

/* No Coprocessor Exception */
.global _isr7
_isr7:
	cli
	push $0
	push $7
	jmp isr_common_stub

/* Double Fault Exception */
.global _isr8
_isr8:
	cli
	/* has error code */
	push $8
	jmp isr_common_stub

/* Coprocessor Segment Overrun Exception */
.global _isr9
_isr9:
	cli
	push $0
	push $9
	jmp isr_common_stub

/* Bad TSS Exception */
.global _isr10
_isr10:
	cli
	/* has error code */
	push $10
	jmp isr_common_stub

/* Segment Not Present Exception */
.global _isr11
_isr11:
	cli
	/* has error code */
	push $11
	jmp isr_common_stub

/* Stack Fault Exception */
.global _isr12
_isr12:
	cli
	/* has error code */
	push $12
	jmp isr_common_stub

/* General Protection Fault Exception */
.global _isr13
_isr13:
	cli
	/* has error code */
	push $13
	jmp isr_common_stub

/* Page Fault Exception */
.global _isr14
_isr14:
	cli
	/* has error code */
	push $14
	jmp isr_common_stub

/* Unknown Interrupt Exception */
.global _isr15
_isr15:
	cli
	push $0
	push $15
	jmp isr_common_stub

/* Coprocessor Fault Exception */
.global _isr16
_isr16:
	cli
	push $0
	push $16
	jmp isr_common_stub

/* Alignment Check Exception (486+) */
.global _isr17
_isr17:
	cli
	push $0
	push $17
	jmp isr_common_stub

/* Machine Check Exception (Pentium/586+) */
.global _isr18
_isr18:
	cli
	push $0
	push $18
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr19
_isr19:
	cli
	push $0
	push $19
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr20
_isr20:
	cli
	push $0
	push $20
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr21
_isr21:
	cli
	push $0
	push $21
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr22
_isr22:
	cli
	push $0
	push $22
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr23
_isr23:
	cli
	push $0
	push $23
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr24
_isr24:
	cli
	push $0
	push $24
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr25
_isr25:
	cli
	push $0
	push $25
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr26
_isr26:
	cli
	push $0
	push $26
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr27
_isr27:
	cli
	push $0
	push $27
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr28
_isr28:
	cli
	push $0
	push $28
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr29
_isr29:
	cli
	push $0
	push $29
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr30
_isr30:
	cli
	push $0
	push $30
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr31
_isr31:
	cli
	push $0
	push $31
	jmp isr_common_stub

/* syscall interrupt */
.global _isr_syscall
_isr_syscall:
	cli
	push $0
	push __isr_syscall_no /* defined in isr.cpp */
	jmp isr_common_stub

.global isr_common_stub
/* This is our common ISR stub. It saves the processor state, sets
    up for kernel mode segments, calls the C-level fault handler,
    and finally restores the stack frame. */
isr_common_stub:
    push %ds
    push %es
    push %fs
    push %gs
    pusha
    mov $0x10, %ax   /* Load the Kernel Data Segment descriptor! */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %esp, %eax
    push %eax
    call _fault_handler
    pop %eax
    // pop gpr's
    popa
    // pop segment registers
    pop %gs
    pop %fs
    pop %es
    pop %ds
    add $8, %esp     /* Cleans up the pushed error code and pushed ISR number */
    iret           /* pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP! */

/* IRQs */

/*
.global _irq0
_irq0:
	cli
	push $0
	push $32
	jmp irq_common_stub
*/

.global _irq1
_irq1:
	cli
	push $0
	push $33
	jmp irq_common_stub

.global _irq2
_irq2:
	cli
	push $0
	push $34
	jmp irq_common_stub

.global _irq3
_irq3:
	cli
	push $0
	push $35
	jmp irq_common_stub

.global _irq4
_irq4:
	cli
	push $0
	push $36
	jmp irq_common_stub

.global _irq5
_irq5:
	cli
	push $0
	push $37
	jmp irq_common_stub

.global _irq6
_irq6:
	cli
	push $0
	push $38
	jmp irq_common_stub

.global _irq7
_irq7:
	cli
	push $0
	push $39
	jmp irq_common_stub

.global _irq8
_irq8:
	cli
	push $0
	push $40
	jmp irq_common_stub

.global _irq9
_irq9:
	cli
	push $0
	push $41
	jmp irq_common_stub

.global _irq10
_irq10:
	cli
	push $0
	push $42
	jmp irq_common_stub

.global _irq11
_irq11:
	cli
	push $0
	push $43
	jmp irq_common_stub

.global _irq12
_irq12:
	cli
	push $0
	push $44
	jmp irq_common_stub

.global _irq13
_irq13:
	cli
	push $0
	push $45
	jmp irq_common_stub

.global _irq14
_irq14:
	cli
	push $0
	push $46
	jmp irq_common_stub

.global _irq15
_irq15:
	cli
	push $0
	push $47
	jmp irq_common_stub

.global irq_common_stub

/* This is a stub that we have created for IRQ based ISRs. This calls
 '_irq_handler' in our C code. We need to create this in an 'irq.c' */
irq_common_stub:
    push %ds
    push %es
    push %fs
    push %gs
    pusha
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %esp, %eax // our register struct starts from here
    push %eax
    call _irq_handler
    pop %eax
    popa
    pop %gs
    pop %fs
    pop %es
    pop %ds
    add $0x8, %esp // cleanup error code and irq number
    iret

.global _irq0
_irq0:
    // we can come here from yield(), which would mean
    // that the timer interrupts are still enabled,
    // so, the PIT can issue another interrupt while
    // this one is active. but since this is registered
    // as an interrupt gate, cpu should already disable
    // the interrupts for us.
	push $0
	push $32
    push %ds
    push %es
    push %fs
    push %gs
    pusha
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %esp, %eax // our register struct starts from here
    push %eax
    call scheduler_scheduleNext // change pointers and whatnot
    mov %eax, %esp // restore the old stack pointer
    sub $32, %esp // pusha pushed the sp before the instruction, so point to the last pushed item
    // pop gpr's
    popa
    // pop segment registers
    pop %gs
    pop %fs
    pop %es
    pop %ds
    cmpl $0x5441534b, (%esp) // check if this is a new task switch
    je prepareNewTask
    add $0x8, %esp // pop error code and irq number
    // save eax
    push %eax
    mov $0x20, %al // finish the irq
    out %al, $0x20
    // restore eax back
    pop %eax
    iret // pop eip, cs, eflags and return

// prepares the stack for a task to be inited
prepareNewTask:
    add $0x8, %esp // pop error code and irq number
    pop %edi   // store eip in edi, callee saved
    pop %ecx // store cs in ecx, caller saved
    add $0x4, %esp // skip eflags for now, they are gonna be 0x200 anyway
    // now, we're pointing to the location which
    // contains the number of arguments to the
    // initializer routine of this task.
    // our arguments start from offset 4 * 17 from
    // here, because we pushed 17 elements on the stack.
    // if we have zero arguments, we have nothing to do
    mov %esp, %eax // store where we are now
    mov %eax, %esi // make a backup in a callee saved register
    mov (%esp), %ebx
    cmp $0, %ebx
    je initTask
    // we have some arguments, so call memmove now to move
    // them here
    // first, prepare the stack for memmove, avoiding our arguments
    sub $68, %esp // arg start
    imul $4, %ebx // arg size in bytes
    sub %ebx, %esp // stack for memmove should start from here
    add $4, %esp // this is our first argument, so this is the source
    mov %esp, %edx // save the source
    sub %ebx, %eax // decrease the original sp by ebx bytes, which is our dest
    add $4, %eax // first arg should be moved here wrt to the original sp
    // save eax
    push %eax
    // save ecx
    push %ecx
    // arguments for memmove
    push %ebx // size
    push %edx // source
    push %eax // this is our dest
    // memmove may make some of our registers garbage,
    // but since we are starting a new task, we're
    // not too much worried about that for now
    call memmove // move now
    // now arguments of the task are at their positions,
    // so pop the arguments for memove
    add $12, %esp
    // pop ecx
    pop %ecx
    // pop the modified sp
    pop %eax
    // set sp to the modified sp
    mov %eax, %esp
initTask:
    // push our return address
    push $leaveTask
    // now the stack is ready
    // push eip, cs, eflags in order
    push $0x200
    push %ecx
    push %edi
    // zero them out
    xor %ebx, %ebx
    xor %edi, %edi
    // esi stores the original esp, so don't clean it out
    // clear out all other registers
    xor %eax, %eax
    xor %ecx, %ecx
    xor %edx, %edx
    mov $0x20, %al // finish the irq
    out %al, $0x20
    xor %eax, %eax
    iret // restore eip, eflags, cs and jump

.global leaveTask
leaveTask:
    // restore the original esp
    mov %esi, %esp
    // pop the first argument
    add $0x4, %esp
    // at this point, eax contains the result, if any.
    // esp contains the address of Future<x>::set,
    // esp + 4 contains a dummy slot for the return
    // address of set, esp + 8 contains the address of the
    // future itself as the first argument, and esp + 12
    // contains an empty slot to push the eax register.
    // so push the eax register to esp + 12
    mov %eax, 0xc(%esp)
    // point to the return address
    add $0x4, %esp
    // finally, jmp to set
    jmp *-0x4(%esp)

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
