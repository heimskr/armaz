COMPILER    ?= aarch64-none-elf-g++
CPPSRC      := $(shell find src/*.cpp)
ASMSRC      := $(shell find asm/*.S)
OBJECTS     := $(CPPSRC:.cpp=.o) $(ASMSRC:.S=.o)
CPPFLAGS    := -Wall -Wextra -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles -D_GNU_SOURCE -Iinclude \
               -Imusl/arch/aarch64 -Imusl/arch/generic -Imusl/obj/src/internal -Imusl/src/include -Imusl/src/internal -Imusl/obj/include -Imusl/include
BIN         := kernel8.elf
IMAGE       := kernel8.img

QEMU_MAIN	?= -nographic -M raspi3 -m 1G -cpu max -smp 4 -drive file=disk.img,format=raw -kernel kernel8.img

.PHONY: all clean run

all: $(IMAGE)

asm/%.o: asm/%.S
	$(COMPILER) $(CPPFLAGS) -c $< -o $@

%.o: %.cpp
	$(COMPILER) $(CPPFLAGS) -c $< -o $@

$(IMAGE): $(OBJECTS)
	aarch64-none-elf-ld -nostdlib -nostartfiles $(OBJECTS) -T link.ld -o $(BIN)
	aarch64-none-elf-objcopy -O binary $(BIN) $(IMAGE)

clean:
	rm -f **/*.o *.o $(BIN) $(IMAGE)

run: $(IMAGE)
	qemu-system-aarch64 $(QEMU_MAIN) $(QEMU_EXTRA)

DEPFILE  := .dep
DEPTOKEN := "\# MAKEDEPENDS"

depend:
	@ echo $(DEPTOKEN) > $(DEPFILE)
	makedepend -f $(DEPFILE) -s $(DEPTOKEN) -- $(COMPILER) $(CPPFLAGS) -- $(CPPSRC) 2>/dev/null
	@ rm $(DEPFILE).bak

sinclude $(DEPFILE)
