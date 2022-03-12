IDT_DESC_SIZE: equ 16
KERNEL_CODE64: equ 8

; params: vec entry selector flags
%macro IDT_ENTRY 4
    ; rbx = &IDT[vec]
    lea rbx, idt + %1 * IDT_DESC_SIZE

    ; first dword = (segment selector << 16) | (entry & 0xFFFF)
    lea rax, _irq_entry_%2
    and eax, 0xFFFF
    or  eax, %3 << 16
    mov dword [rbx], eax

    ; second dword = (entry & 0xFFFF0000) | (flags << 8)
    lea rax, _irq_entry_%2
    and eax, 0xFFFF0000
    or  eax, (%4 << 8)
    mov dword [rbx + 4], eax

    ; third dword = entry >> 32
    lea rax, _irq_entry_%2
    shr rax, 32
    mov dword [ebx + 8], eax

    ; fourth dword = reserved
    mov dword [ebx + 16], 0
%endmacro

%define ERRCODE 1
%define NOERRCODE 0

%define GATE_INTERRUPT 0b10001110
%define GATE_TRAP      0b10001111

; params: errcode entry
%macro IRQ_ENTRY 2
    section .text
    extern %2
    align 16

_irq_entry_%2:
%if !%1
    ; Push dummy error code
    push qword 0
%endif

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
    call %2

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

%if %1
    ; Skip error code.
    add rsp, 8
%endif

    ; Return from interrupt.
    iretq
%endmacro

section .bss
    idt:
        resb 4 * 256

section .data
    idt_ptr:
        dw 4 * 256
        dd idt

section .text
    extern ud_handler
    extern df_handler
    extern ts_handler
    extern np_handler
    extern ss_handler
    extern gp_handler
    extern pf_handler
    extern spurious_handler
    extern timer_handler


    IRQ_ENTRY ERRCODE,   ud_handler
    IRQ_ENTRY ERRCODE,   df_handler
    IRQ_ENTRY ERRCODE,   ts_handler
    IRQ_ENTRY ERRCODE,   np_handler
    IRQ_ENTRY ERRCODE,   ss_handler
    IRQ_ENTRY ERRCODE,   gp_handler
    IRQ_ENTRY ERRCODE,   pf_handler
    IRQ_ENTRY NOERRCODE, nm_handler
    IRQ_ENTRY NOERRCODE, spurious_handler
    IRQ_ENTRY NOERRCODE, timer_handler

    global irq_init
    irq_init:
        push rbp
        mov rbp, rsp
        
        ;         Vec   C handler          Selector       Flags
        IDT_ENTRY 6,    ud_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 7,    nm_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 8,    df_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 10,   ts_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 11,   np_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 12,   ss_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 13,   gp_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 14,   pf_handler,        KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 32,   timer_handler,     KERNEL_CODE64, GATE_INTERRUPT
        IDT_ENTRY 39,   spurious_handler,  KERNEL_CODE64, GATE_INTERRUPT

        lidt [idt_ptr]

        mov rsp, rbp
        pop rbp
        ret
