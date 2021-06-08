.global memcpy
memcpy:
push %edi
push %esi
mov 0xc(%esp), %edi // dest
mov %edi, %eax // return value
mov 0x10(%esp), %esi // source
mov 0x14(%esp), %edx // size
mov %edx, %ecx
shr $2, %ecx // convert to 4byte chunk count
cld // increment pointers successively
rep movsl // copy in 4byte chunks
// find the remaining bytes
mov %edx, %ecx
and $0x3, %ecx
rep movsb // copy in 1byte chunks
pop %esi
pop %edi
ret
