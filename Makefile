SHELL := /bin/bash

AS=nasm
CC=x86_64-elf-gcc
LD=x86_64-elf-ld
OBJCOPY=x86_64-elf-objcopy
GRUB_MKRESCUE=grub-mkrescue

ROOT=$(shell pwd)
ASFLAGS=-f elf64 -g -F dwarf
CCFLAGS=-I$(ROOT) -mno-mmx -mno-sse -mno-sse2 -maddress-mode=long -mcmodel=kernel -g -m64 -mno-red-zone -ffreestanding -fno-common -Wall -Wextra -Werror -nostdlib
LDFLAGS=-nostdlib --no-dynamic-linker --warn-constructors --warn-common --no-eh-frame-hdr

ifndef RELEASE
CCFLAGS+=-DQEMU_PIT_HACK
endif

export

QEMU=qemu-system-x86_64
QEMUFLAGS=-cdrom kernel.iso -monitor stdio -accel kvm

ifdef EFI
QEMUFLAGS+=-bios /usr/share/OVMF/x64/OVMF.fd
endif

kernel.iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp grub.cfg isodir/boot/grub
	cp kernel.bin isodir/boot
	$(GRUB_MKRESCUE) -o kernel.iso isodir
	rm -rf isodir

kernel.bin:
	$(MAKE) -C boot/
	$(MAKE) -C drivers/
	$(MAKE) -C kernel/
	$(MAKE) -C mm/
	$(LD) $(LDFLAGS) -T <(cpp -P -E linker.ld) `find $(ROOT) -name '*.o'` -o kernel.bin
	$(OBJCOPY) --only-keep-debug kernel.bin kernel.sym
	$(OBJCOPY) --strip-debug kernel.bin

qemu: kernel.iso
	$(QEMU) $(QEMUFLAGS)

qemu-gdb: kernel.iso
	$(QEMU) $(QEMUFLAGS) -s -S

clean:
	$(MAKE) -C boot/ clean
	$(MAKE) -C drivers/ clean
	$(MAKE) -C kernel/ clean
	$(MAKE) -C mm/ clean
	rm -f kernel.bin
	rm -f kernel.sym
	rm -f kernel.iso

.PHONY: kernel.bin clean

