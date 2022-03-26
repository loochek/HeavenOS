#include "drivers/apic.h"
#include "kernel/panic.h"
#include "kernel/irq.h"
#include "arch/x86.h"

// APIC timer period in milliseconds
#define APIC_TIMER_PERIOD 1

// APIC callibration period in milliseconds
#define CALLIBRATE_PERIOD 10

// APIC

#define TYPE_LAPIC          0
#define TYPE_IOAPIC         1
#define TYPE_ISO            2
#define TYPE_NMI            3
#define TYPE_LAPIC_OVERRIDE 4

#define FLAGS_ACTIVE_LOW      2
#define FLAGS_LEVEL_TRIGGERED 8

#define APIC_ID          0x20
#define APIC_VER         0x30
#define APIC_TASKPRIOR   0x80
#define APIC_EOI         0x0B0
#define APIC_LDR         0x0D0
#define APIC_DFR         0x0E0
#define APIC_SPURIOUS    0x0F0
#define APIC_ESR         0x280
#define APIC_ICRL        0x300
#define APIC_ICRH        0x310
#define APIC_LVT_TMR     0x320
#define APIC_LVT_PERF    0x340
#define APIC_LVT_LINT0   0x350
#define APIC_LVT_LINT1   0x360
#define APIC_LVT_ERR     0x370
#define APIC_TMRINITCNT  0x380
#define APIC_TMRCURRCNT  0x390
#define APIC_TMRDIV      0x3E0
#define APIC_LAST        0x38F
#define APIC_DISABLE     0x10000
#define APIC_SW_ENABLE   0x100
#define APIC_CPUFOCUS    0x200
#define APIC_NMI         (4<<8)
#define APIC_INIT        0x500
#define APIC_BCAST       0x80000
#define APIC_LEVEL       0x8000
#define APIC_DELIVS      0x1000
#define TMR_PERIODIC     0x20000
#define TMR_BASEDIV      (1<<20)
#define TMR_DIV_X1       0xB

#define IOAPIC_REG_TABLE  0x10

// PIT
#define CMD_BINARY     0x00 // Use Binary counter values
#define PIT_COUNTER2   0x42
#define PIT_CMD        0x43
#define PIT_GATE       0x61
#define CMD_MODE1      0x02 // Hardware Retriggerable One-Shot
#define CMD_RW_BOTH    0x30 // Least followed by Most Significant Byte
#define CMD_COUNTER2   0x80
#define CMD_CH2_IN     0x01
#define CMD_CH2_OUT    0x20
#define PIT_FREQUENCY  1193182

typedef struct ioapic
{
    uint32_t reg;
    uint32_t pad[3];
    uint32_t data;
} ioapic_t;

typedef struct __attribute__((packed)) madt_entry
{
    uint8_t type;
    uint8_t length;
    uint8_t data[0];
} madt_entry_t;

typedef struct __attribute__((packed)) madt_header
{
    acpi_sdt_header_t acpi;
    uint32_t lapic_addr;
    uint32_t flags;
    madt_entry_t first_entry;
} madt_header_t;

volatile uint32_t* lapic_ptr = NULL;
volatile ioapic_t* ioapic_ptr = NULL;

static void lapic_write(size_t idx, uint32_t value)
{
    lapic_ptr[idx / 4] = value;
}

static uint32_t lapic_read(size_t idx)
{
    return lapic_ptr[idx / 4];
}

// static void ioapic_write(int reg, uint32_t data)
// {
//     ioapic_ptr->reg = reg;
//     ioapic_ptr->data = data;
// }

// static void ioapic_enable(int irq, int target_irq)
// {
//     ioapic_write(IOAPIC_REG_TABLE + 2 * irq, target_irq);
//     ioapic_write(IOAPIC_REG_TABLE + 2 * irq + 1, 0);
// }

void apic_init()
{
    // Find Multiple APIC Description Table, it contains addresses of I/O APIC and LAPIC.
    madt_header_t* header = (madt_header_t*)acpi_lookup("APIC");
    if (!header)
        panic("ACPI MADT not found!");

    lapic_ptr = (volatile uint32_t*)(uint64_t)header->lapic_addr;
    
    madt_entry_t* entry = &header->first_entry;

    for (;;)
    {
        if ((uint8_t*)entry >= (uint8_t*)header + header->acpi.length)
            break;

        switch (entry->type)
        {
            case TYPE_LAPIC:
                break;

            case TYPE_IOAPIC:
                ioapic_ptr = (volatile ioapic_t*)(uint64_t)(*(uint32_t*)(&entry->data[2]));
                break;
        }

        entry = (struct madt_entry*)((uint8_t*)entry + entry->length);
    }

    if (!ioapic_ptr)
        panic("Cannot locate I/O APIC address");

    if (!lapic_ptr)
        panic("Cannot locate Local APIC address");

    // Disable old PIC.
    outb(0x20 + 1, 0xFF);
    outb(0xA0 + 1, 0xFF);

    // Enable APIC, by setting spurious interrupt vector and APIC Software Enabled/Disabled flag.
    lapic_write(APIC_SPURIOUS, IRQ_SPURIOUS | APIC_SW_ENABLE);

    // Disable performance monitoring counters.
    lapic_write(APIC_LVT_PERF, APIC_DISABLE);

    // Disable local interrupt pins.
    lapic_write(APIC_LVT_LINT0, APIC_DISABLE);
    lapic_write(APIC_LVT_LINT1, APIC_DISABLE);

    // Signal EOI.
    lapic_write(APIC_EOI, 0);

    // Set highest priority for current task.
    lapic_write(APIC_TASKPRIOR, 0);
}

void apic_setup_timer()
{
    // Setup APIC timer for callibration
    lapic_write(APIC_LVT_TMR, IRQ_TIMER);
    lapic_write(APIC_TMRDIV, TMR_DIV_X1);

    // Using PIT to callibrate APIC timer
    // PIT Channel 2 setup
    outb(PIT_CMD, CMD_BINARY | CMD_MODE1 | CMD_RW_BOTH | CMD_COUNTER2);

    int pit_divisor = PIT_FREQUENCY * CALLIBRATE_PERIOD / 1000;
    outb(PIT_COUNTER2, pit_divisor);
    outb(PIT_COUNTER2, pit_divisor >> 8);
    
    // ---- Measure start

    // PIT gate low
    outb(PIT_GATE, inb(PIT_GATE) & (~CMD_CH2_IN));
    // PIT gate high
    outb(PIT_GATE, inb(PIT_GATE) | CMD_CH2_IN);
    // PIT is counting now
    // Reset APIC timer counter
    lapic_write(APIC_TMRINITCNT, -1);
    // Wait PIT gate to be high
#ifndef QEMU_PIT_HACK
    while (!(inb(PIT_GATE) & CMD_CH2_OUT));
#else
    while (inb(PIT_GATE) & CMD_CH2_OUT);
#endif
    // Disable APIC timer
    lapic_write(APIC_LVT_TMR, APIC_DISABLE);

    // ---- Measure end

    uint64_t cpu_bus_freq = (uint64_t)((uint32_t)(-1) - lapic_read(APIC_TMRCURRCNT)) * 1000 / CALLIBRATE_PERIOD;

    // Final APIC timer setup using calculated CPU bus frequency
    lapic_write(APIC_TMRINITCNT, cpu_bus_freq * APIC_TIMER_PERIOD / 1000);
    lapic_write(APIC_TMRDIV, TMR_DIV_X1);
    lapic_write(APIC_LVT_TMR, IRQ_TIMER | TMR_PERIODIC);
}

void apic_eoi()
{
    lapic_write(APIC_EOI, 0);
}
