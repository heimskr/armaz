# COMPILER    ?= aarch64-none-elf-g++ -nostartfiles
COMPILER    ?= clang++ --target=aarch64-elf
# LINKER      ?= aarch64-none-elf-ld -nostartfiles
LINKER      ?= ld.lld-11
CPPSRC      := $(shell find src -name \*.cpp)
ASMSRC      := $(shell find asm/*.S)
OBJECTS     := $(CPPSRC:.cpp=.o) $(ASMSRC:.S=.o)
RPIFLAGS    := -march=armv8-a+crc -mcpu=cortex-a72 -DRASPPI=4 -mno-unaligned-access
CMNFLAGS    := -Wall -Wextra -Wno-asm-operand-widths -g -O2 -std=c++20 -ffreestanding -nostdinc -nostdlib -D_GNU_SOURCE -Iinclude $(RPIFLAGS) \
               -Imusl/arch/aarch64 -Imusl/arch/generic -Imusl/obj/src/internal -Imusl/src/include -Imusl/src/internal -Imusl/obj/include -Imusl/include
CPPFLAGS    := $(CMNFLAGS) -fno-exceptions -fno-rtti -std=c++2a -Iinclude/lib/libcxx -Drestrict=__restrict__
BIN         := kernel8.elf
IMAGE       := kernel8.img
LIBS        := musl/lib/libc.a lib/libgcc.a

QEMU_MAIN	?= -nographic -M raspi3 -m 1G -cpu max -smp 4 -drive file=disk.img,format=raw -kernel kernel8.img

.PHONY: all clean run

all: $(IMAGE)

asm/%.o: asm/%.S
	$(COMPILER) $(CMNFLAGS) -c $< -o $@

%.o: %.cpp
	$(COMPILER) $(CPPFLAGS) -c $< -o $@

$(IMAGE): $(OBJECTS) $(LIBS)
	$(LINKER) -nostdlib $(OBJECTS) $(LIBS) -T link.ld -o $(BIN)
	aarch64-none-elf-objcopy -O binary $(BIN) $(IMAGE)

musl/lib/libc.a:
	$(MAKE) -C musl

clean:
	rm -f *.o asm/*.o `find src -iname "*.o"` $(BIN) $(IMAGE)

run: $(IMAGE)
	qemu-system-aarch64 $(QEMU_MAIN) $(QEMU_EXTRA)

DEPFILE  := .dep
DEPTOKEN := "\# MAKEDEPENDS"

depend:
	@ echo $(DEPTOKEN) > $(DEPFILE)
	makedepend -f $(DEPFILE) -s $(DEPTOKEN) -- $(COMPILER) $(CPPFLAGS) -- $(CPPSRC) 2>/dev/null
	@ rm $(DEPFILE).bak

sinclude $(DEPFILE)
