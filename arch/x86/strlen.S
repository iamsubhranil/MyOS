.global strlen
strlen:
push %edi
mov 0x8(%esp), %edi
xor %ecx, %ecx // clear
not %ecx // max length
xor %eax, %eax // 0 is the byte to be searched
repne scasb // scan per byte
not %ecx // 1's complement the result to get the length
sub $1, %ecx // exclude the null byte
mov %ecx, %eax // store it to eax
pop %edi
ret
