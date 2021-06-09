.global memcpy
memcpy:
cld // increment pointers successively
push %edi
push %esi
mov 0xc(%esp), %edi // dest
mov %edi, %eax // return value
mov 0x10(%esp), %esi // source
mov 0x14(%esp), %edx // size
mov %edx, %ecx
// find first 0-3 bytes
mov %edx, %ecx
and $0x3, %ecx
rep movsb // copy in 1byte chunks
// move the original size
mov %edx, %ecx
shr $2, %ecx // convert to 4byte chunk count
jz memcpy_return // if we don't have any 4byte chunks, return
rep movsl // copy in 4byte chunks
memcpy_return:
pop %esi
pop %edi
ret
