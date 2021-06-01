MyOS (needs a better name)
----------------------------
Trying my hands on implementing an OS. Currently GDT, ISR, IRQs, Paging, Heap 
and Multitasking are implemented. The kernel also operates in higher half.

## Requirements
Requires i686-elf-g++ and i686-elf-as for compilation.
Required `qemu` for virtualization. You can also use `myos.iso` to boot anywhere.

## Building
```
$ make release # also tries to start qemu
```
For debug builds, do
```
$ make debug
```
For debugging with GDB, do
```
$ make debug_gdb
$ make gdbstart
```
