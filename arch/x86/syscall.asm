extern _current
extern do_syscall

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

; params: pop_rax
%macro POP_REGS 1
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
%endif
    add rsp, 8
%endmacro

section .bss
    saved_rsp:
        resb 8

; This is an entry point for syscall instruction.
; On enter, following holds:
;   rax contains syscall number;
;   rdi, rsi, rdx, rcx, r8, r9 contain syscall arguments (in order);
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

        ; TODO: construct arch_regs_t on stack and call do_syscall.


        ; Restore user-space rsp.
        mov rsp, qword [saved_rsp]

        sysretq

    global pop_and_iret
    pop_and_iret:
        POP_REGS
        ; Skip error code.
        add rsp, 8
        iretq
