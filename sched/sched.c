#include <stdbool.h>
#include <linker.h>
#include <arch/x86/x86.h>
#include <kernel/irq.h>
#include <kernel/panic.h>
#include <mm/frame_alloc.h>
#include <mm/obj.h>
#include <mm/paging.h>
#include <sched/sched.h>

#define PREEMPT_TICKS 10

uint64_t sched_timer = 0;

task_t tasks[MAX_TASK_COUNT] = {};
extern void jump_userspace();

static int setup_vmem(vmem_t* vm)
{
    // Setup direct physical mapping.
    uint8_t* virt_addr = (uint8_t*)KERNEL_DIRECT_PHYS_MAPPING_START;
    uint8_t* phys_addr = (uint8_t*)0;
    for (size_t i = 0; i < (KERNEL_DIRECT_PHYS_MAPPING_SIZE / GB); i++)
    {
        int err = vmem_map_page_1gb(vm, virt_addr, phys_addr, VMEM_WRITE);
        if (err < 0)
            return err;

        virt_addr += GB;
        phys_addr += GB;
    }

    // Setup kernel sections mapping.
    uint8_t* virt_addr_curr = (uint8_t*)KERNEL_SECTIONS_START;
    uint8_t* phys_addr_curr = (uint8_t*)&_phys_start_hh;
    uint8_t* phys_addr_end = (uint8_t*)&_phys_end_kernel_sections;
    while (phys_addr_curr < phys_addr_end)
    {
        int err = vmem_map_page_2mb(vm, virt_addr_curr, phys_addr_curr, VMEM_WRITE);
        if (err < 0)
            return err;

        phys_addr_curr += 2 * MB;
        virt_addr_curr += 2 * MB;
    }

    // Setup user-space code.
    int err = vmem_map_page(vm, (void*)0x10000, &_phys_start_user, VMEM_USER);
    if (err < 0)
        return err;

    return 0;
}

static int setup_init_task()
{
    task_t* new_task = sched_allocate_task();
    if (new_task == NULL)
        return -ENOMEM;

    int err = vmem_init(&new_task->vmem);
    if (err < 0)
        return err;

    err = setup_vmem(&new_task->vmem);
    if (err < 0)
        return err;

    // Setup user-space stack.
    err = vmem_alloc_pages(&new_task->vmem, (void*)0x70000000, 4, VMEM_USER | VMEM_WRITE);
    if (err < 0)
        return err;

    new_task->state = TASK_RUNNABLE;

    err = arch_thread_new(&new_task->arch_thread, NULL);
    if (err < 0)
        return err;
    
    return 0;
}

task_t* _current = NULL;

static arch_thread_t sched_context = {};
static vmem_t sched_vmem = {};

static void sched_switch_to(task_t* next)
{
    sched_current() = next;
    // Set CPU time dealdline for the task
    sched_current()->preempt_deadline = sched_timer + PREEMPT_TICKS;

    vmem_switch_to(&next->vmem);
    arch_thread_switch(&sched_context, &next->arch_thread);
}

void sched_start()
{
    // Interrupts are still disabled.
    if (setup_init_task() < 0)
        panic("cannot allocate init task");

    vmem_init_from_current(&sched_vmem);

    irq_enable();

    for (;;)
    {
        bool found = false;
        for (size_t i = 0; i < MAX_TASK_COUNT; i++)
        {
            // Make sleeping task runnable again if it's time
            if (tasks[i].state == TASK_SLEEPING && sched_timer >= tasks[i].sleep_until)
                tasks[i].state = TASK_RUNNABLE;

            if (tasks[i].state == TASK_RUNNABLE)
            {
                // We found running task, switch to it.
                sched_switch_to(&tasks[i]);
                found = 1;
                // We've returned to the scheduler.

                if (tasks[i].state == TASK_ZOMBIE)
                {
                    // Free resources occupied by dying task
                    vmem_destroy(&tasks[i].vmem);
                    arch_thread_destroy(&tasks[i].arch_thread);
                }
            }
        }

        if (!found)
        {
            // If we didn't found a runnable task, wait for next interrupt and retry scheduling.
            x86_hlt();
        }
    }
}

void sched_switch()
{
    kassert(_current != NULL);
    task_t* prev = sched_current();
    sched_current() = NULL;
    arch_thread_switch(&prev->arch_thread, &sched_context);
}

void sched_timer_tick()
{
    sched_timer++;

    // Return if we are not in user task now
    if (!_current)
        return;

    // We should switch task after some timer ticks
    if (sched_current()->preempt_deadline >= sched_timer)
    {
        // Task CPU time exceeded - switch to others
        sched_switch();
        // Returned to this task at the moment
    }
}

void sched_yield()
{
    panic_on_reach();
}

task_t* sched_allocate_task()
{
    uint64_t curr_pid = 1;
    while (tasks[curr_pid].state != TASK_NOT_ALLOCATED && curr_pid < MAX_TASK_COUNT)
        curr_pid++;

    if (curr_pid == MAX_TASK_COUNT)
        return NULL;

    tasks[curr_pid].pid = curr_pid;
    return &tasks[curr_pid];
}
