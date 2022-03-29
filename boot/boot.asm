%include "multiboot.asm"

KERNEL_DIRECT_PHYS_MAPPING_START: equ 0xffff888000000000

PTE_P:   equ 0x001   ; Present
PTE_W:   equ 0x002   ; Writeable
PTE_U:   equ 0x004   ; User
PTE_PWT: equ 0x008   ; Write-Through
PTE_PCD: equ 0x010   ; Cache-Disable
PTE_A:   equ 0x020   ; Accessed
PTE_D:   equ 0x040   ; Dirty
PTE_PS:  equ 0x080   ; Page Size
PTE_G:   equ 0x100   ; Global
PTE_NX:  equ 1 << 63 ; Not executable

GDT_G:  equ 1 << 23
GDT_L:  equ 1 << 21
GDT_P:  equ 1 << 15
GDT_S:  equ 1 << 12
GDT_E:  equ 1 << 11
GDT_RW: equ 1 << 9

GDT_KERNEL_CODE64_SELECTOR: equ 0x8
GDT_KERNEL_DATA64_SELECTOR: equ 0x10

PAGE_SIZE: equ 4096

; Kernel entry point code
section .early.text
    ; C code entry point
    extern early_init

    ; _early_entry_point is an entry point of our kernel, execution after bootloader starts here.
    global _early_entry_point
    _early_entry_point:
    ; Tell assembler, that we are running 32-bit protected mode.
    bits 32
   ; Save Multiboot boot info pointer

        mov dword [mb_early_boot_info], ebx
        mov dword [mb_early_boot_info + 4], 0

   ; To enable long mode, we need to setup early paging.
   ; Here we setup identity mapping for the first gigabyte.

   ; PML4[0] = PDPT writable
        mov edi, early_pml4
        mov eax, early_pdpt
        or eax, PTE_P | PTE_W
        mov dword [edi], eax

   ; PDPT[0] = 0 writable
        mov edi, early_pdpt
        mov dword [edi], PTE_PS | PTE_P | PTE_W

   ; Enabling 64-bit mode is described in ISDM, Volume 3A, Section 9.8.5.

   ; Write PML4 physical address into CR3.
        mov eax, early_pml4
        mov cr3, eax

   ; Set PAE bit in CR4.
        mov eax, cr4
        or eax, 1 << 5
        mov cr4, eax

   ; Set IA32_EFER.LME bit (Long Mode Enable).
        mov ecx, 0xC0000080
        rdmsr
        or eax, 1 << 8
        wrmsr

   ; Enable paging by setting PG bit in CR0.
        mov eax, cr0
        or eax, 1 << 31
        mov cr0, eax

   ; Load new GDT.
        lgdt [gdt64_ptr]
        jmp GDT_KERNEL_CODE64_SELECTOR:.gdt_code_64

    .gdt_code_64:
   ; We we are now running 64-bit mode. Yay!

    bits 64
   ; Setup segment registers
        mov ax, GDT_KERNEL_DATA64_SELECTOR
        mov ds, ax
        mov ss, ax
        mov es, ax
        mov fs, ax
        mov gs, ax

   ; Setup simple stack and call early_init.
        mov rsp, early_stack
        call early_init

   ; Halt processor, i.e. put it in a such state that next instruction will be executed only after interrupt.
   ; Effectively it means inifinite non-busy loop.
    .loop:
        hlt
        jmp .loop

     extern kmain
     extern kmain_return
     extern early_data

     global jump_to_kernel_main
     jump_to_kernel_main:
        ; Because KERNEL_DIRECT_PHYS_MAPPING_START is not representable as imm32, we need move it to register first.
        mov rbx, KERNEL_DIRECT_PHYS_MAPPING_START

        ; Reset to the same stack, but use its virtual address instead.
        lea rsp, early_stack
        add rsp, rbx

        ; If kernel_main occasionally returns, kmain_return will be called.
        lea rax, kmain_return
        push rax

        ; Use lea to indicate that we want use imm64.
        lea rax, kmain
        lea rdi, early_data
        add rdi, rbx
        jmp rax

     ; Called when kmain occasionally returns
     ; Just halt
     kmain_return:
     .loop:
        hlt
        jmp .loop

section .early.data
    ; Setup GDT for 64-bit mode.
    align 4096
    gdt64:
        ; 0: null segment.
        dd 0
        dd 0

        ; 1: 64-bit kernel code segment.
        dd 0xFFFF
        ; (0xF << 16) fills base 31:24.
        dd GDT_G | GDT_L | GDT_P | GDT_S | GDT_E | GDT_RW | (0xF << 16)

        ; 2: 64-bit kernel data segment.
        dd 0xFFFF
        ; (0xF << 16) fills base 31:24.
        dd GDT_G | GDT_L | GDT_P | GDT_S | GDT_RW | (0xF << 16)

    gdt64_ptr:
        dw $ - gdt64
        dq gdt64

    ; Multiboot boot info pointer
    global mb_early_boot_info
    mb_early_boot_info: dq 0

section .early.bss
    global early_pml4

    align PAGE_SIZE
    early_pml4:  resb PAGE_SIZE
    early_pdpt:  resb PAGE_SIZE
     
    resb PAGE_SIZE * 4
    early_stack:
