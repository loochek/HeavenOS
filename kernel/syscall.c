#include <kernel/syscall.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <arch/x86/arch.h>
#include <sched/sched.h>
#include <common.h>

static int64_t sys_sleep (arch_regs_t* regs);
static int64_t sys_fork  (arch_regs_t* regs);
static int64_t sys_getpid(arch_regs_t* regs);
static int64_t sys_exit  (arch_regs_t* regs);
static int64_t sys_wait  (arch_regs_t* regs);

static syscall_fn_t syscall_table[] =
{
    [SYS_SLEEP] = sys_sleep,
    [SYS_FORK] = sys_fork,
    [SYS_GETPID] = sys_getpid,
    [SYS_EXIT] = sys_exit,
    [SYS_WAIT] = sys_wait
};

uint64_t do_syscall(uint64_t sysno, arch_regs_t* regs)
{
    if (sysno >= SYS_MAX)
        return -ENOSYS;

    syscall_fn_t syscall = syscall_table[sysno];
    return syscall(regs);
}

static int64_t sys_sleep(arch_regs_t* regs)
{
    // Prepare to sleep
    uint64_t ms = syscall_arg0(regs);
    sched_current()->sleep_until = sched_timer + ms / SCHED_TIMER_PERIOD;
    sched_current()->state = TASK_SLEEPING;

    // Go back to scheduler
    sched_switch();
    // Returned to this task at the moment
    return 0;
}

static int64_t sys_getpid(arch_regs_t* regs)
{
    UNUSED(regs);
    return _current->pid;
}

static int64_t sys_fork(arch_regs_t* parent_regs)
{
    (void)parent_regs;

    task_t* child = sched_allocate_task();
    int res = vmem_clone(&child->vmem, &sched_current()->vmem);
    if (res < 0)
        return res;

    res = arch_thread_clone(&child->arch_thread, NULL, &sched_current()->arch_thread);
    if (res < 0)
        return res;

    child->ppid = sched_current()->pid;
    child->state = TASK_RUNNABLE;
    return 0;
}

static _Noreturn int64_t sys_exit(arch_regs_t* regs)
{
    sched_current()->exitcode = syscall_arg0(regs);
    sched_current()->state = TASK_ZOMBIE;

    // Notify parent about child's death
    task_t* parent = &tasks[sched_current()->ppid];
    if (parent->state == TASK_WAITING && parent->wait_pid == sched_current()->pid)
        parent->state = TASK_RUNNABLE;

    // Go back to scheduler
    sched_switch();
    // Scheduler mustn't schedule this task anymore
    panic_on_reach();

    // Just to satisfy _Noreturn
    while (true);
}

static int64_t sys_wait(arch_regs_t* regs)
{
    size_t pid = syscall_arg0(regs);
    if (tasks[pid].ppid != sched_current()->pid)
        return -ECHILD;

    if (tasks[pid].state != TASK_ZOMBIE)
    {
        // Child is not dead yet
        // Go back to scheduler
        sched_current()->state = TASK_WAITING;
        sched_current()->wait_pid = pid;
        sched_switch();

        // Returned to this task - child must me zombie at the moment
        kassert_dbg(tasks[pid].state == TASK_ZOMBIE);
    }

    /// TODO: check status to be writeable by user
    int* status = (int*)syscall_arg1(regs);
    *status = tasks[pid].exitcode;

    // Free task entry
    tasks[pid].state = TASK_NOT_ALLOCATED;
    return 0;
}
