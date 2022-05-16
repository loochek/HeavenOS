section .text
    global context_switch
    context_switch:
        push rbx
        push rbp
        push r12
        push r13
        push r14
        push r15

        mov qword [rdi], rsp
        mov rsp, qword [rsi]

        pop r15
        pop r14
        pop r13
        pop r12
        pop rbp
        pop rbx
        ret
