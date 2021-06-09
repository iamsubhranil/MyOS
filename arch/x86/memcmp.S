.global memcmp
memcmp:
cld // increment pointers in successive comparison
push %edi
push %esi
mov 0xc(%esp), %esi // s1
mov 0x10(%esp), %edi // s2
mov 0x14(%esp), %edx // size
mov %edx, %ecx
// find out first 0-3 number of bytes
mov %edx, %ecx
and $0x3, %ecx
repz cmpsb // compare in 1byte chunks
jne memcmp_mismatch // if we have already found a mismatch, bail
mov %edx, %ecx // restore the original size
shr $0x2, %ecx // convert byte count to 4byte count
repz cmpsl // compare in 4byte chunks
jne memcmp_mismatch // if we found mismatch, bail
mov $0, %eax
jmp memcmp_return

memcmp_mismatch:
jc memcmp_less
mov $1, %eax // greater
jmp memcmp_return

memcmp_less:
mov $-1, %eax
// jmp memcmp_return

memcmp_return:
pop %esi
pop %edi
ret
