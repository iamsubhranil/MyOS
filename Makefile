CXX=i686-elf-g++
LD=i686-elf-ld
CXXFLAGS=-Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -ffreestanding -I. -std=c++17
LDFLAGS=-Xlinker -Map=kernel.map
QEMUFLAGS=

SRCS := $(wildcard *.cpp */*.cpp */*/*.cpp */*/*/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

ASMSRCS := $(wildcard *.S */*.S */*/*.S */*/*/*.S)
ASMOBJS := $(patsubst %.S,%.o,$(ASMSRCS))

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

%.o: %.S
	$(CXX) -c $< -o $@ $(CXXFLAGS)

linker: linker.ld $(OBJS) $(ASMOBJS)
	$(CXX) -T linker.ld -o myos.elf -ffreestanding $(CXXFLAGS) $(LDFLAGS) -nostdlib $(OBJS) $(ASMOBJS) -lgcc

iso: linker
	cp myos.elf isodir/boot/myos.elf
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir
	qemu-system-i386 -cdrom myos.iso $(QEMUFLAGS)

start: myos.elf
	qemu-system-i386 -kernel myos.elf $(QEMUFLAGS)

gdbstart:
	gdb \
	-ex "file myos.elf" \
	-ex 'set arch auto' \
	-ex 'target remote localhost:1234'

clean:
	$(RM) -f $(ASMOBJS) $(OBJS) myos.elf

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CFLAGS) -MM $^>>./.depend;

distclean: clean
	$(RM) *~ .depend

# include .depend

all: release
release: CXXFLAGS += -O2
release: QEMUFLAGS = -serial stdio
release: linker start

debug: CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
debug: QEMUFLAGS += -no-shutdown -no-reboot -nographic -serial mon:stdio
debug: linker start

debug_iso: CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
debug_iso: QEMUFLAGS = -no-shutdown -no-reboot -nographic  -serial mon:stdio -S -s
debug_iso: linker iso

debug_gdb: QEMUFLAGS += -S -s
debug_gdb: debug

dis:
	objdump -lSCwr -j .text --visualize-jumps=extended-color --disassemble="$(sym)" myos.elf
