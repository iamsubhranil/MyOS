CXX=i686-elf-g++
LD=i686-elf-ld

# $(wildcard *.cpp /xxx/xxx/*.cpp): get all .cpp files from the current directory and dir "/xxx/xxx/"
SRCS := $(wildcard *.cpp)
# $(patsubst %.cpp,%.o,$(SRCS)): substitute all ".cpp" file name strings to ".o" file name strings
OBJS := $(patsubst %.cpp,%.o,$(SRCS))

boot: boot.s
	i686-elf-as boot.s -o boot.o

%.o: %.cpp
	$(CXX) -c $< -o $@ -ffreestanding -O2 -Wall -Wextra

linker: linker.ld $(OBJS) 
	$(CXX) -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib $(OBJS) boot.o -lgcc

iso: myos.bin
	cp myos.bin isodir/boot/myos.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso isodir

start: myos.bin
	qemu-system-x86_64 -kernel myos.bin

clean:
	$(RM) -f $(OBJS) myos.bin

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CFLAGS) -MM $^>>./.depend;

distclean: clean
	$(RM) *~ .depend

include .depend

all: boot linker start
