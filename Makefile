AS=nasm
CC=clang
LD=clang

AS_FLAGS=-f elf64 -g -F dwarf
CC_FLAGS=-mno-mmx -mno-sse -mno-sse2 -fno-pie -g -mno-red-zone -std=gnu99 -ffreestanding -nostdlib -O2 -Wall -Wextra -Werror -Wnewline-eof -fno-stack-protector
LD_FLAGS=-ffreestanding -O0 -no-pie -nostdlib -fno-stack-protector

OBJCOPY=objcopy
GRUB_MKRESCUE=grub-mkrescue

QEMU=qemu-system-x86_64
QEMU_FLAGS=-cdrom kernel.iso -monitor stdio

ifdef EFI
QEMU_FLAGS+=-bios /usr/share/OVMF/x64/OVMF.fd
endif

kernel.iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub
	cp kernel.bin isodir/boot
	$(GRUB_MKRESCUE) -o kernel.iso isodir
	rm -rf isodir

%.o: %.c
	$(CC) $(CC_FLAGS) -c -o $@ $<

%.o: %.asm
	$(AS) $(AS_FLAGS) -o $@ $<

kernel.bin: boot.o common.o kernel.o multiboot.o fb.o console.o printk.o panic.o acpi.o apic.o irq.o irq_asm.o
	$(LD) $(LD_FLAGS) -T linker.ld -o $@ $^

qemu: kernel.iso
	$(QEMU) $(QEMU_FLAGS)

qemu-gdb: kernel.iso
	$(QEMU) $(QEMU_FLAGS) -s -S

clean:
	rm -f *.o kernel.bin kernel.iso
