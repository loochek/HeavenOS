#ifndef IRQ_H
#define IRQ_H

#include <arch/x86/arch.h>

typedef enum
{
    IRQ_UD       = 6,  // Illegal opcode
    IRQ_NM       = 7,  // No math coprocessorNon-maskable interrupt
    IRQ_DF       = 8,  // Double fault
    IRQ_TS       = 10, // Invalid TSS
    IRQ_NP       = 11, // Segment not present
    IRQ_SS       = 12, // Stack segment fault
    IRQ_GP       = 13, // General protection fault
    IRQ_PF       = 14, // Page fault
    IRQ_TIMER    = 32,
    IRQ_SPURIOUS = 39
} irq_t;

/**
 * Initializes IDT
 */
void irq_init();

static inline void irq_disable()
{
    __asm__ volatile ("cli");
}

static inline void irq_enable()
{
    __asm__ volatile ("sti");
}

#endif
