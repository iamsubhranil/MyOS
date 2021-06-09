// 1byte, this is the original definition of memset
.global memset
memset:
cld // we want to increment the pointers successively
push %edi
mov 0x8(%esp), %edi // dest
mov 0xc(%esp), %eax
// store the size
mov 0x10(%esp), %edx // size
mov %edx, %ecx
// copy the first 0-3 bytes
and $0x3, %ecx
// if we are already aligned, skip this copy
jz memset_big
rep stosb // copy
memset_big:
// restore the original size
mov %edx, %ecx
shr $2, %ecx // convert byte count to 4byte count
// if we're done, return
jz memset_ret
// we have some more bytes to copy
// single byte is passed as argument,
// convert it to 4bytes, so that we can copy 4bytes at once
mov %eax, %edx
shl $0x8, %edx
or %edx,%eax
shl $0x8, %edx
or %edx, %eax
shl $0x8, %edx
or %edx, %eax
// copy in 4byte chunks
rep stosl
memset_ret:
// restore edi
pop %edi
// now return
ret

// copies in 2byte chunk, size must be aligned
.global memsetw
memsetw:
cld // increment pointers
push %edi
mov 0x8(%esp), %edi // dest
mov 0xc(%esp), %eax // value
mov 0x10(%esp), %edx // size in bytes
shr $1, %edx // word count is size in bytes / 2
// copy 0-1 word at first
mov %edx, %ecx
and $0x1, %ecx
jz memsetw_big // we don't have any remaining words, so skip it
movsw // copy remaining word
memsetw_big:
// check if we have remaining 4byte words
mov %edx, %ecx
shr $1, %ecx // convert 2byte word count to 4byte word count
jz memsetw_ret // we don't have any 4byte words, so skip it
// argument word is 2 bytes, convert it to 4bytes, so that we can
// copy 4bytes at once
mov %eax, %edx
shl $0x10, %edx
or %edx, %eax
// finally, copy
rep stosl
memsetw_ret:
// restore edi
pop %edi
// now return
ret

// copies in 4byte chunk, size must be aligned
.global memsetl
memsetl:
push %edi
mov 0x8(%esp), %edi // dest
mov 0xc(%esp), %eax
mov 0x10(%esp), %ecx // size in bytes, convert it to 4byte size
shr $2, %ecx
cld // we want to increment the pointers successively
// copy in 4byte chunks
rep stosl
// restore edi
pop %edi
// now return
ret
