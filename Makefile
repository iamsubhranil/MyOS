CXX=i686-elf-g++
LD=i686-elf-ld
CXXFLAGS=-Wall -Wextra
QEMUFLAGS=

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

boot: boot.s
	i686-elf-as boot.s -o boot.o

%.o: %.cpp
	$(CXX) -c $< -o $@ -ffreestanding $(CXXFLAGS)

linker: linker.ld $(OBJS) 
	$(CXX) -T linker.ld -o myos.bin -ffreestanding $(CXXFLAGS) -nostdlib $(OBJS) boot.o -lgcc

iso: myos.bin
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir

start: myos.bin
	qemu-system-x86_64 -kernel myos.bin $(QEMUFLAGS)

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
release: boot linker start

debug: CXXFLAGS += -O0 -g3
debug: QEMUFLAGS += -no-shutdown -no-reboot -d int -monitor stdio
debug: boot linker start
