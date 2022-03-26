C_SOURCES   := $(wildcard *.c)
ASM_SOURCES := $(wildcard *.asm)
C_OBJS      := $(C_SOURCES:.c=.c.o)
ASM_OBJS    := $(ASM_SOURCES:.asm=.asm.o)

build: $(C_OBJS) $(ASM_OBJS)

%.c.o: %.c
	$(CC) $(CCFLAGS) -c $< -o $@

%.asm.o: %.asm
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f kernel.bin
	rm -f kernel.sym
	rm -f kernel.iso

.PHONY: build clean
