MB_MAGIC: equ 0xE85250D6
MB_ARCH:  equ 0

MB_TAG_END: equ 0
MB_TAG_FB:  equ 5

FB_PREFFERED_WIDTH:  equ 1280
FB_PREFFERED_HEIGHT: equ 720
FB_PREFFERED_DEPTH:  equ 32

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
