.global _tsc_measure_irq
_tsc_measure_irq:
    // increment the freq counter
    add $1, %esi
    // finish the irq
    mov $0x20, %al
    out %al, $0x20
    // return from the interrupt
    iret

.global tsc_measure
// call as u64 res = tsc_measure(times);
// returns total increment of the tsc after 'times' irqs
tsc_measure:
    // back up the registers
    push %esi
    push %edi
    push %ebx
    // init the last tsc
    cpuid
    rdtsc
    // back up
    push %edx // hi
    push %eax // low
    mov 0x18(%esp), %ecx // store the total freq at ecx
    xor %esi, %esi // init esi to 0
    sti // enable interrupts
measure_loop:
    hlt // wait for the irq
    cmp %ecx, %esi // compare
    jne measure_loop // if we have measured 'times' irqs, stop
    cli // disable interrupts for now
    // now find out the ts for now
    cpuid
    rdtsc
    // restore our old timestamp
    pop %ecx // low
    pop %ebx // hi
    // sub
    sub %ecx, %eax
    sbb %ebx, %edx
    // now edx, eax contains the increment in tsc in current_freq hz
    // restore backed up registers
    pop %ebx
    pop %edi
    pop %esi
    // return now
    ret
