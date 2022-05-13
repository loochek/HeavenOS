extern _current
extern do_syscall

RPL_RING3: equ 3
RPL_RING0: equ 0

KERNEL_CODE_SEG: equ (1 << 3) | RPL_RING0
KERNEL_DATA_SEG: equ (2 << 3) | RPL_RING0
USER_DATA_SEG:   equ (3 << 3) | RPL_RING3
USER_CODE_SEG:   equ (4 << 3) | RPL_RING3

%macro PUSH_REGS 0
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
%endmacro

%macro POP_REGS 0
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
%endmacro

section .bss
    saved_rsp:
        resb 8

; This is an entry point for syscall instruction.
; On enter, following holds:
;   rax contains syscall number;
;   rdi, rsi, rdx, r10, r8, r9 contain syscall arguments (in order);
;   rcx contains userspace rip;
;   r11 contains userspace rflags;
;   rsp contains *userspace* stack (it may be corrupted or not mapped);
;   interrupts are disabled (IF set in IA32_FMASK).
section .text
    global syscall_entry
    syscall_entry:
        ; We cannot use user-controlled rsp here:
        ; No stack switch will be performed if exception or interrupt occurs here since we are already in ring0.
        ; So, invalid rsp leads us to the double fault.
        mov qword [saved_rsp], rsp
        ; rsp = _current->arch_thread.kstack_top
        mov rsp, qword [_current]
        mov rsp, qword [rsp]

        ; We have a reliable stack now, enable interrupts.
        sti

        ; Construct arch_regs_t
        ; ss
        push qword USER_DATA_SEG
        ; rsp
        push qword [saved_rsp]
        ; rflags
        push r11
        ; cs
        push qword USER_CODE_SEG
        ; rip
        push rcx
        ; errcode
        push qword 0
        ; irq_num
        push qword 0
        ; General purpose registers
        PUSH_REGS

        ; do_syscall args
        mov rdi, rax
        mov rsi, rsp

        call do_syscall
        ; Return value in rax
        ; Note that caller-saved registers are not affected at all (as do_syscall respects the ABI)
        ; (only rsp, but it's restored on the next line)

        ; Restore user-space rsp.
        mov rsp, qword [saved_rsp]
        sysret

    global pop_and_iret
    pop_and_iret:
        POP_REGS
        ; Skip error code and IRQ number
        add rsp, 16
        iretq