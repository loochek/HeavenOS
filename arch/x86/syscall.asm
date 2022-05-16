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

; args: restore_rax=true
%macro POP_REGS 0-1 1
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

%if %1
    pop rax
%else
    add rsp, 8
%endif
%endmacro

section .bss
    rsp_scratch_space:
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
        mov qword [rsp_scratch_space], rsp
        ; rsp = _current->arch_thread.kstack_top
        mov rsp, qword [_current]
        mov rsp, qword [rsp]

        ; We have a reliable stack now

        ; Save task state (arch_regs_t) on the stack
        ; ss
        push qword USER_DATA_SEG
        ; rsp
        push qword [rsp_scratch_space]
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

        ; It's now safe to enable interrupts
        sti

        ; do_syscall args
        mov rdi, rax
        mov rsi, rsp

        call do_syscall

        ; Restore task state (arch_regs_t) on the stack
        ; Restore general purpose registers except rax (it's already do_syscall return value)
        POP_REGS 0
        ; Skip irq_num and errcode
        add rsp, 16
        ; Restore RIP (sysret expects it in rcx)
        pop rcx
        ; Ignore cs
        add rsp, 8
        ; Restore RFLAGS (sysret expects it in r11)
        pop r11
        ; Disable interrupts before stack switch
        cli
        ; Restore user stack
        mov rsp, qword [rsp]
        ; Return to user task
        o64 sysret

    global pop_and_iret
    pop_and_iret:
        ; Restore general purpose registers
        POP_REGS
        ; Skip error code and IRQ number
        add rsp, 16
        ; Jump to user task
        iretq
