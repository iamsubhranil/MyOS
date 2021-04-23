/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
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

/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
.section .text
.global _start
.type _start, @function
_start:
	/*
	The bootloader has loaded us into 32-bit protected mode on a x86
	machine. Interrupts are disabled. Paging is disabled. The processor
	state is as defined in the multiboot standard. The kernel has full
	control of the CPU. The kernel can only make use of hardware features
	and any code it provides as part of itself. There's no printf
	function, unless the kernel provides its own <stdio.h> header and a
	printf implementation. There are no security restrictions, no
	safeguards, no debugging mechanisms, only what the kernel provides
	itself. It has absolute and complete power over the
	machine.
	*/

	/*
	To set up a stack, we set the esp register to point to the top of our
	stack (as it grows downwards on x86 systems). This is necessarily done
	in assembly as languages such as C cannot function without a stack.
	*/
	mov $stack_top, %esp

	/*
	This is a good place to initialize crucial processor state before the
	high-level kernel is entered. It's best to minimize the early
	environment where crucial features are offline. Note that the
	processor is not fully initialized yet: Features such as floating
	point instructions and instruction set extensions are not initialized
	yet. The GDT should be loaded here. Paging should be enabled here.
	C++ features such as global constructors and exceptions will require
	runtime support to work as well.
	*/
	/* kernelMain does that by calling appropriate functions */

	/*
	Enter the high-level kernel. The ABI requires the stack is 16-byte
	aligned at the time of the call instruction (which afterwards pushes
	the return pointer of size 4 bytes). The stack was originally 16-byte
	aligned above and we've since pushed a multiple of 16 bytes to the
	stack since (pushed 0 bytes so far) and the alignment is thus
	preserved and the call is well defined.
	*/
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

/* IDTs */
/* Division By Zero Exception */
.global _isr0
_isr0:

	push $0
	push $0
	jmp isr_common_stub

/* Debug Exception */
.global _isr1
_isr1:

	push $0
	push $1
	jmp isr_common_stub

/* Non Maskable Interrupt Exception */
.global _isr2
_isr2:

	push $0
	push $2
	jmp isr_common_stub

/* Breakpoint Exception */
.global _isr3
_isr3:

	push $0
	push $3
	jmp isr_common_stub

/* Into Detected Overflow Exception */
.global _isr4
_isr4:

	push $0
	push $4
	jmp isr_common_stub

/* Out of Bounds Exception */
.global _isr5
_isr5:

	push $0
	push $5
	jmp isr_common_stub

/* Invalid Opcode Exception */
.global _isr6
_isr6:

	push $0
	push $6
	jmp isr_common_stub

/* No Coprocessor Exception */
.global _isr7
_isr7:

	push $0
	push $7
	jmp isr_common_stub

/* Double Fault Exception */
.global _isr8
_isr8:

	/* has error code */
	push $8
	jmp isr_common_stub

/* Coprocessor Segment Overrun Exception */
.global _isr9
_isr9:

	push $0
	push $9
	jmp isr_common_stub

/* Bad TSS Exception */
.global _isr10
_isr10:

	/* has error code */
	push $10
	jmp isr_common_stub

/* Segment Not Present Exception */
.global _isr11
_isr11:

	/* has error code */
	push $11
	jmp isr_common_stub

/* Stack Fault Exception */
.global _isr12
_isr12:

	/* has error code */
	push $12
	jmp isr_common_stub

/* General Protection Fault Exception */
.global _isr13
_isr13:

	/* has error code */
	push $13
	jmp isr_common_stub

/* Page Fault Exception */
.global _isr14
_isr14:

	/* has error code */
	push $14
	jmp isr_common_stub

/* Unknown Interrupt Exception */
.global _isr15
_isr15:

	push $0
	push $15
	jmp isr_common_stub

/* Coprocessor Fault Exception */
.global _isr16
_isr16:

	push $0
	push $16
	jmp isr_common_stub

/* Alignment Check Exception (486+) */
.global _isr17
_isr17:

	push $0
	push $17
	jmp isr_common_stub

/* Machine Check Exception (Pentium/586+) */
.global _isr18
_isr18:

	push $0
	push $18
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr19
_isr19:

	push $0
	push $19
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr20
_isr20:

	push $0
	push $20
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr21
_isr21:

	push $0
	push $21
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr22
_isr22:

	push $0
	push $22
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr23
_isr23:

	push $0
	push $23
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr24
_isr24:

	push $0
	push $24
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr25
_isr25:

	push $0
	push $25
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr26
_isr26:

	push $0
	push $26
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr27
_isr27:

	push $0
	push $27
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr28
_isr28:

	push $0
	push $28
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr29
_isr29:

	push $0
	push $29
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr30
_isr30:

	push $0
	push $30
	jmp isr_common_stub

/* Reserved Exceptions */
.global _isr31
_isr31:

	push $0
	push $31
	jmp isr_common_stub

.global isr_common_stub
/* This is our common ISR stub. It saves the processor state, sets
    up for kernel mode segments, calls the C-level fault handler,
    and finally restores the stack frame. */
isr_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax   /* Load the Kernel Data Segment descriptor! */
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %esp, %eax   /* Push us the stack */
    push %eax
    //mov _fault_handler, %eax
    //call *%eax       /* A special call, preserves the 'eip' register */
    call _fault_handler
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $8, %esp     /* Cleans up the pushed error code and pushed ISR number */
    iret           /* pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP! */

/* IRQs */

.global _irq0
_irq0:

	push $0
	push $32
	jmp irq_common_stub

.global _irq1
_irq1:

	push $0
	push $33
	jmp irq_common_stub

.global _irq2
_irq2:

	push $0
	push $34
	jmp irq_common_stub

.global _irq3
_irq3:

	push $0
	push $35
	jmp irq_common_stub

.global _irq4
_irq4:

	push $0
	push $36
	jmp irq_common_stub

.global _irq5
_irq5:

	push $0
	push $37
	jmp irq_common_stub

.global _irq6
_irq6:

	push $0
	push $38
	jmp irq_common_stub

.global _irq7
_irq7:

	push $0
	push $39
	jmp irq_common_stub

.global _irq8
_irq8:

	push $0
	push $40
	jmp irq_common_stub

.global _irq9
_irq9:

	push $0
	push $41
	jmp irq_common_stub

.global _irq10
_irq10:

	push $0
	push $42
	jmp irq_common_stub

.global _irq11
_irq11:

	push $0
	push $43
	jmp irq_common_stub

.global _irq12
_irq12:

	push $0
	push $44
	jmp irq_common_stub

.global _irq13
_irq13:

	push $0
	push $45
	jmp irq_common_stub

.global _irq14
_irq14:

	push $0
	push $46
	jmp irq_common_stub

.global _irq15
_irq15:

	push $0
	push $47
	jmp irq_common_stub

.global irq_common_stub

/* This is a stub that we have created for IRQ based ISRs. This calls
 '_irq_handler' in our C code. We need to create this in an 'irq.c' */
irq_common_stub:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %esp, %eax
    push %eax
    //mov _irq_handler, %eax
    call _irq_handler
    pop %eax
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    add $0x8, %esp
    iret

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start
