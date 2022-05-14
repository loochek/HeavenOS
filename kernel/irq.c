#include "kernel/irq.h"
#include "kernel/panic.h"
#include "arch/x86.h"
#include "drivers/apic.h"
#include "mm/vmem.h"

static const char *exc_names[] =
{
    "Divide error",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Unknown exception",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection",
    "Page Fault",
    "Unknown exception",
    "x87 FPU Floating-Point Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception"
};

static void timer_handler();
static void spurious_handler();
static void dump(struct irqctx* ctx);
static const char *get_irq_name(int irq_num);

void irq_handler(struct irqctx* ctx)
{
    if (ctx->irq_num == IRQ_PF)
    {
        if (vmem_handle_pf((void*)x86_read_cr2()))
            return;
    }

    switch (ctx->irq_num)
    {
    case IRQ_TIMER:
        timer_handler(ctx);
        break;

    case IRQ_SPURIOUS:
        spurious_handler(ctx);
        break;
    
    default:
        dump(ctx);
        panic("Unhandled IRQ");
        break;
    }
}

static void timer_handler()
{
    printk(".");
    apic_eoi();
}

static void spurious_handler()
{
    apic_eoi();
}

static void dump(struct irqctx* ctx)
{
    printk("Unhandled IRQ %d [%s]\n", ctx->irq_num, get_irq_name(ctx->irq_num));
    printk("Error code: %d\n", ctx->errcode);

    if (ctx->irq_num == IRQ_PF)
        printk("Fault address: %p\n", x86_read_cr2());

    printk("RAX: 0x%x RBX: 0x%x\n"
           "RCX: 0x%x RDX: 0x%x\n"
           "RSI: 0x%x RDI: 0x%x\n"
           "R8: 0x%x R9: 0x%x\n"
           "R10: 0x%x R11: 0x%x\n"
           "R12: 0x%x R13: 0x%x\n"
           "R14: 0x%x R15: 0x%x\n"
           "RIP: 0x%x CS: 0x%x\n",
           ctx->reg_rax, ctx->reg_rbx, ctx->reg_rcx, ctx->reg_rdx, ctx->reg_rsi, ctx->reg_rdi, ctx->reg_r8,
           ctx->reg_r9, ctx->reg_r10, ctx->reg_r11, ctx->reg_r12, ctx->reg_r13, ctx->reg_r14, ctx->reg_r15,
           ctx->reg_rip, ctx->reg_cs);
}

static const char *get_irq_name(int irq_num)
{
    if (irq_num <= (int)(sizeof(exc_names) / sizeof(exc_names[0])))
        return exc_names[irq_num];

    switch (irq_num)
    {
    case IRQ_TIMER:
        return "Timer";
    
    case IRQ_SPURIOUS:
        return "Spurious";
    
    default:
        return "Unknown IRQ";
    }
}
