IDT_DESC_SIZE: equ 16
KERNEL_CODE64: equ 8

; params: vec selector flags
%macro IDT_ENTRY 3
    ; rbx = &IDT[vec]
    lea rbx, [rel idt + %1 * IDT_DESC_SIZE]

    ; first dword = (segment selector << 16) | (entry & 0xFFFF)
    lea rax, [rel _irq_entry_%1]
    and eax, 0xFFFF
    or  eax, %2 << 16
    mov dword [rbx], eax

    ; second dword = (entry & 0xFFFF0000) | (flags << 8)
    lea rax, [rel _irq_entry_%1]
    and eax, 0xFFFF0000
    or  eax, (%3 << 8)
    mov dword [rbx + 4], eax

    ; third dword = entry >> 32
    lea rax, [rel _irq_entry_%1]
    shr rax, 32
    mov dword [rbx + 8], eax

    ; fourth dword = reserved
    mov dword [rbx + 16], 0
%endmacro

%define ERRCODE 1
%define NOERRCODE 0

%define GATE_INTERRUPT 0b10001110
%define GATE_TRAP      0b10001111

; params: vector errcode
%macro IRQ_ENTRY 2
    section .text
    align 16

_irq_entry_%1:
%if !%2
    ; Push dummy error code
    push qword 0
%endif

    ; Push IRQ number
    push qword %1

    ; First of all, save GPRs on stack.
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Pass pointer for struct irqctx* which is current stack top.
    mov rdi, rsp
    call irq_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    ; Skip error code and IRQ number.
    add rsp, 16

    ; Return from interrupt.
    iretq
%endmacro

section .bss
    idt:
        resb IDT_DESC_SIZE * 256

section .data
    idt_ptr:
        dw IDT_DESC_SIZE * 256
        dq idt

section .text
    extern irq_handler

    ; Interrupt handlers
    ;         Vec Errcode present
    IRQ_ENTRY 6,  ERRCODE
    IRQ_ENTRY 7,  NOERRCODE
    IRQ_ENTRY 8,  ERRCODE
    IRQ_ENTRY 10, ERRCODE
    IRQ_ENTRY 11, ERRCODE
    IRQ_ENTRY 12, ERRCODE
    IRQ_ENTRY 13, ERRCODE
    IRQ_ENTRY 14, ERRCODE
    IRQ_ENTRY 32, NOERRCODE
    IRQ_ENTRY 39, NOERRCODE

    global irq_init
    irq_init:
        push rbp
        mov rbp, rsp
        
        ; IDT entries
        ;         Vec Selector       Flags
        IDT_ENTRY 6,  KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 7,  KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 8,  KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 10, KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 11, KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 12, KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 13, KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 14, KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 32, KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 39, KERNEL_CODE64, GATE_INTERRUPT

        lidt [rel idt_ptr]

        mov rsp, rbp
        pop rbp
        ret
