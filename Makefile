CXX=i686-elf-g++
LD=i686-elf-ld
CXXFLAGS=-Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -ffreestanding -I. -std=c++17
QEMUFLAGS=

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp */*.cpp */*/*.cpp */*/*/*.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

boot: arch/x86/boot.S
	$(CXX) $(CXXFLAGS) arch/x86/boot.S -c -o boot.o

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

linker: linker.ld $(OBJS) 
	$(CXX) -T linker.ld -o myos.bin -ffreestanding $(CXXFLAGS) -nostdlib $(OBJS) boot.o -lgcc

iso: linker
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir
	qemu-system-i386 -cdrom myos.iso $(QEMUFLAGS)

start: myos.bin
	qemu-system-i386 -kernel myos.bin $(QEMUFLAGS)

gdbstart:
	gdb \
    -ex "file myos.bin" \
	-ex 'set arch auto' \
    -ex 'target remote localhost:1234'

clean:
	$(RM) -f $(OBJS) myos.bin

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CFLAGS) -MM $^>>./.depend;

distclean: clean
	$(RM) *~ .depend

include .depend

all: release
release: CXXFLAGS += -O2
release: QEMUFLAGS = -serial stdio
release: boot linker start

debug: CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
debug: QEMUFLAGS += -no-shutdown -no-reboot -serial mon:stdio
debug: boot linker start

debug_iso: CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
debug_iso: QEMUFLAGS = -no-shutdown -no-reboot -serial mon:stdio -S -s
debug_iso: boot linker iso

debug_gdb: QEMUFLAGS += -S -s
debug_gdb: debug

dis:
	objdump -lSCwr -j .text --visualize-jumps=extended-color --disassemble="$(sym)" myos.bin
