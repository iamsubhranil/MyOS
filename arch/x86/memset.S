// 1byte, this is the original definition of memset
.global memset
memset:
push %edi
mov 0x8(%esp), %edi // dest
mov 0xc(%esp), %eax // byte
mov 0x10(%esp), %edx // size
mov %edx, %ecx
shr $2, %ecx // convert byte count to 4byte count
cld // we want to increment the pointers successively
// copy in 4byte chunks
rep stosl
// find the remaining bytes
mov %edx, %ecx
and $0x3, %ecx
// now copy the remaining, there is really no benefit
// of trying to copy in 2byte chunks, since we
// have at most 4bytes left.
rep stosb
// restore edi
pop %edi
// now return
ret
