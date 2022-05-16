#include <arch/x86/irq.h>
#include <kernel/panic.h>
#include <arch/x86/x86.h>
#include <drivers/apic.h>
#include <mm/vmem.h>
#include <sched/sched.h>

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
static void dump(arch_regs_t* ctx);
static const char *get_irq_name(int irq_num);

void irq_handler(arch_regs_t* ctx)
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
    apic_eoi();
    sched_timer_tick();
}

static void spurious_handler()
{
    apic_eoi();
}

static void dump(arch_regs_t* ctx)
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
           ctx->rax, ctx->rbx, ctx->rcx, ctx->rdx, ctx->rsi, ctx->rdi, ctx->r8,
           ctx->r9, ctx->r10, ctx->r11, ctx->r12, ctx->r13, ctx->r14, ctx->r15,
           ctx->rip, ctx->cs);
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
