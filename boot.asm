MB_MAGIC: equ 0xE85250D6
MB_ARCH:  equ 0

MB_TAG_END: equ 0
MB_TAG_FB:  equ 5

FB_PREFFERED_WIDTH:  equ 1280
FB_PREFFERED_HEIGHT: equ 720
FB_PREFFERED_DEPTH:  equ 32

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

GDT_P: equ 1 << 15
GDT_S: equ 1 << 12
GDT_G: equ 1 << 23
GDT_L: equ 1 << 21

; .multiboot section defines Multiboot 2 compatible header.
section .multiboot
    .header_start:
    ; Header magic fields
        align 8
        dd MB_MAGIC
        dd MB_ARCH
        dd .header_end - .header_start ; Header length
        dd -(MB_MAGIC + MB_ARCH + .header_end - .header_start)

    ; Framebuffer tag
        align 8
        dw MB_TAG_FB
        dw 0
        dd 20
        dd FB_PREFFERED_WIDTH
        dd FB_PREFFERED_HEIGHT
        dd FB_PREFFERED_DEPTH

    ; End tag
        align 8
        dw MB_TAG_END
        dw 0
        dd 8

    .header_end:

; Kernel entry point code
section .text
    ; C code entry point
    extern kmain 

    ; _start is an entry point of our kernel, execution after bootloader starts here.
    global _start
    _start:
    ; Tell assembler, that we are running 32-bit protected mode.
    bits 32
   ; Save Multiboot boot info pointer

        mov dword [mb_boot_info], ebx
        mov dword [mb_boot_info + 4], 0

   ; To enable long mode, we need to setup basic paging.
   ; Here we setup identity mapping for the first 4 gigabytes.

   ; PML4[0] = PDPT writable
        mov edi, pml4
        mov eax, pdpt
        or eax, PTE_P | PTE_W
        mov dword [edi], eax

   ; PDPT[0] = 0 writable
        mov edi, pdpt
        mov dword [edi], PTE_PS | PTE_P | PTE_W

   ; PDPT[1] = 1GB writable
        mov edi, pdpt + 8 * 1
        mov dword [edi], 0x40000000 | PTE_PS | PTE_P | PTE_W

   ; PDPT[2] = 2GB writable
        mov edi, pdpt + 8 * 2
        mov dword [edi], 0x80000000 | PTE_PS | PTE_P | PTE_W

   ; PDPT[3] = 3GB writable
        mov edi, pdpt + 8 * 3
        mov dword [edi], 0xC0000000 | PTE_PS | PTE_P | PTE_W

   ; Enabling 64-bit mode is described in ISDM, Volume 3A, Section 9.8.5.

   ; Write PML4 physical address into CR3.
        mov eax, pml4
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
        jmp 0x8:.gdt_code_64

    .gdt_code_64:
   ; We we are now running 64-bit mode. Yay!

    bits 64
   ; Setup simple stack and call kernel_main.
        mov rsp, bootstack
        call kmain

   ; Halt processor, i.e. put it in a such state that next instruction will be executed only after interrupt.
   ; Effectively it means inifinite non-busy loop.
    .loop:
        hlt
        jmp .loop

section .data
    ; Setup GDT for 64-bit mode.
    align 4096
    gdt64:
        ; 0: null segment.
        dd 0
        dd 0

        ; 1: 64-bit code segment.
        dd 0xFFFF
        ; (0xF << 16) fills base 31:24.
        dd GDT_G | GDT_L | GDT_P| GDT_S | (1 << 11) | (1 << 9) | (0xF << 16)

    gdt64_ptr:
        dw $ - gdt64
        dd gdt64

    ; Multiboot boot info pointer
    global mb_boot_info
    mb_boot_info: dq 0

section .bss
    align 4096
    pml4:      resb 4096
    pdpt:      resb 4096
    bootstack: resb 4096