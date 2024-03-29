CXX=i686-elf-g++
LD=i686-elf-ld
CXXFLAGS=-Wall -Wextra -fno-exceptions -fno-rtti -nostdlib -ffreestanding -I. -std=c++17
QEMUFLAGS=

SRCS := $(wildcard *.cpp */*.cpp */*/*.cpp */*/*/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

ASMSRCS := $(wildcard *.S */*.S */*/*.S */*/*/*.S)
ASMOBJS := $(patsubst %.S,%.o,$(ASMSRCS))

ISODIR=isodir/

%.o: %.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)

%.o: %.S
	$(CXX) -c $< -o $@ $(CXXFLAGS)

linker: linker.ld $(OBJS) $(ASMOBJS)
	$(CXX) -T linker.ld -o myos.bin -ffreestanding $(CXXFLAGS) -nostdlib $(OBJS) $(ASMOBJS) -lgcc

iso: CXXFLAGS += -O2 -g2
iso: linker
	mkdir -p $(ISODIR)/boot/grub
	cp myos.bin isodir/boot/
	cp grub.cfg isodir/boot/grub/
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
	$(RM) -f $(ASMOBJS) $(OBJS) myos.bin
	$(RM) -rf $(ISODIR)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CFLAGS) -MM $^>>./.depend;

distclean: clean
	$(RM) *~ .depend

# include .depend

all: release
release: CXXFLAGS += -O2 -g2
release: QEMUFLAGS = -serial stdio
release: linker start

debug: CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
debug: QEMUFLAGS += -no-shutdown -no-reboot -nographic -serial mon:stdio
debug: linker start

debug_iso: CXXFLAGS += -O0 -g3 -fno-omit-frame-pointer
debug_iso: QEMUFLAGS = -no-shutdown -no-reboot -nographic -serial mon:stdio -S -s
debug_iso: linker iso

debug_gdb: QEMUFLAGS += -S -s
debug_gdb: debug

dis:
	objdump -lSCwr -j .text --visualize-jumps=extended-color --disassemble="$(sym)" myos.bin
