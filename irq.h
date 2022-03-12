#ifndef IRQ_H
#define IRQ_H

#include "common.h"

#define GATE_INTERRUPT 0b10001110
#define GATE_TRAP      0b10001111

void irq_init();

static inline void irq_disable() {
    asm volatile ("cli");
}

static inline void irq_enable() {
    asm volatile ("sti");
}

// irqctx_errcode represent interrupt handler frame.
typedef struct __attribute__((packed)) irqctx {
    // Pushed by IRQ entry
    uint64_t reg_r15;
    uint64_t reg_r14;
    uint64_t reg_r13;
    uint64_t reg_r12;
    uint64_t reg_r11;
    uint64_t reg_r10;
    uint64_t reg_r9;
    uint64_t reg_r8;
    uint64_t reg_rbp;
    uint64_t reg_rdi;
    uint64_t reg_rsi;
    uint64_t reg_rdx;
    uint64_t reg_rcx;
    uint64_t reg_rbx;
    uint64_t reg_rax;
    // Hardware IRQ stack frame
    uint64_t errcode;
    uint64_t reg_rip;
    uint16_t reg_cs;
    uint16_t __pad1;
    uint32_t __pad2;
    uint64_t rflags;
    uint64_t reg_rsp;
    uint16_t reg_ss;
    uint16_t __pad3;
    uint32_t __pad4;
} irqctx_t;

#endif
