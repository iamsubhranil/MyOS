# MyOS (needs a better name)
----------------------------
Trying my hands on implementing an OS. Currently GDT, ISR and IRQs are implemented.

## Requirements
Requires i686-elf-g++ and i686-elf-as for compilation.
Required `qemu` for virtualization. You can also use `myos.iso` to boot anywhere.

## Building
```
$ make all # also tries to start qemu
```
For debug builds, do
```
$ make debug
```
