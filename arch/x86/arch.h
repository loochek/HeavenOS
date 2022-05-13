#ifndef ARCH_H
#define ARCH_H

#include <stdint.h>
#include <arch/x86/context_switch.h>
#include <arch/x86/syscall.h>

typedef struct __attribute__((packed))
{
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t irq_num;
    // Hardware IRQ stack frame
    uint64_t errcode;
    uint64_t rip;
    uint16_t cs;
    uint16_t __pad1;
    uint32_t __pad2;
    uint64_t rflags;
    uint64_t rsp;
    uint16_t ss;
    uint16_t __pad3;
    uint32_t __pad4;
} arch_regs_t;

#define arch_regs_set_retval(regs, retval) (regs)->rax = (retval)
#define arch_regs_copy(dst, src) memcpy(dst, src, sizeof(arch_regs_t))

typedef struct
{
    uint8_t* kstack_top;
    uint8_t saved_rsp;
    context_t context;
} arch_thread_t;

void arch_init();
int arch_thread_new(arch_thread_t* thread, arch_regs_t** regs);
int arch_thread_clone(arch_thread_t* dst, arch_regs_t** regs, arch_thread_t* src);
void arch_thread_destroy(arch_thread_t* thread);
void arch_thread_switch(arch_thread_t* prev, arch_thread_t* next);

#endif
