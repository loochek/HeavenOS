#ifndef SCHED_H
#define SCHED_H

#include <stddef.h>

#include <arch/x86/arch.h>
#include <mm/vmem.h>

// Timer period in milliseconds
#define SCHED_TIMER_PERIOD 1

#define MAX_TASK_COUNT (1 << 16)

typedef enum state
{
    TASK_NOT_ALLOCATED = 0,
    TASK_RUNNABLE      = 1,
    TASK_WAITING       = 2,
    TASK_ZOMBIE        = 3,
    TASK_SLEEPING      = 4
} state_t;

typedef struct task
{
    // arch_thread_t must be the first member.
    arch_thread_t arch_thread;
    size_t pid;
    size_t ppid;
    size_t wait_pid;
    state_t state;
    uint64_t flags;
    uint64_t preempt_deadline;
    uint64_t sleep_until;
    vmem_t vmem;
    int exitcode;
} task_t;

extern uint64_t sched_timer;

extern task_t tasks[];

extern task_t* _current;
#define sched_current() _current

/**
 * Starts scheduling
 */
void sched_start();

/**
 * Switches back to scheduler context
 */
void sched_switch();

/**
 * Preemptive multitasking provider
 */
void sched_timer_tick();

/**
 * Allocates task entry
 * 
 * \return Task entry
 */
task_t* sched_allocate_task();

#endif
