.global memcmp
memcmp:
push %edi
push %esi
mov 0xc(%esp), %esi // s1
mov 0x10(%esp), %edi // s2
mov 0x14(%esp), %edx // size
mov %edx, %ecx
shr $0x2, %ecx // convert byte count to 4byte count
cld // increment pointers in successive comparison
repz cmpsl // compare in 32byte chunks
jne memcmp_mismatch // if we already found mismatch, bail
// s1 and s2 are equal till now, continue the compare
// find out the remaining number of bytes
mov %edx, %ecx
and $0x7, %ecx
repz cmpsb // compare in 1byte chunks
jne memcmp_mismatch
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
