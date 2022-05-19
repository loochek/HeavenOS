# HeavenOS

Simple educational operation system. Based on [HeLL OS](https://github.com/carzil/hellos), written as part of the [mipt-llp-2022](https://github.com/carzil/mipt-llp-2022) course.

## Building and running
Requires GCC cross-compiler (can be replaced with Clang), NASM and `grub-mkrescue` for creating bootable ISO.
```
# Build kernel and ISO for QEMU
make
# Build and run
make qemu
# Build kernel and ISO (disables some QEMU hacks - to run under VMware and on real hardware)
make RELEASE=1
```

If you use x86-64 PC, Linux and GRUB, you can try HeavenOS on your PC: `sudo mv kernel.bin /boot/kernel.bin`, then, in GRUB command line: `multiboot2 /boot/kernel.bin` and `boot`.
