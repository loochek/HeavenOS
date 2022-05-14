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
    UNUSED(parent_regs);

    task_t* child = sched_allocate_task();

    int res = vmem_clone(&child->vmem, &sched_current()->vmem);
    if (res < 0)
        return res;

    arch_regs_t* child_regs = NULL;
    res = arch_thread_clone(&child->arch_thread, &child_regs, &sched_current()->arch_thread);
    if (res < 0)
        return res;

    child->ppid = sched_current()->pid;
    child->state = TASK_RUNNABLE;

    child_regs->rax = 0;
    return child->pid;
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
    size_t child_pid = syscall_arg0(regs);
    if (tasks[child_pid].ppid != sched_current()->pid)
        return -ECHILD;

    if (tasks[child_pid].state != TASK_ZOMBIE)
    {
        // Child is not dead yet
        // Go back to scheduler
        sched_current()->state = TASK_WAITING;
        sched_current()->wait_pid = child_pid;
        sched_switch();

        // Returned to this task - child must me zombie at the moment
        kassert_dbg(tasks[child_pid].state == TASK_ZOMBIE);
    }

    int* status = (int*)syscall_arg1(regs);
    if (status != NULL)
    {
        vmem_area_t *area = vmem_is_mapped(&sched_current()->vmem, status);
        if (area == NULL || (area->flags & (VMEM_USER | VMEM_WRITE)) == 0)
        {
            printk("sys_wait: status addr is not valid - terminating pid %d\n", sched_current()->pid);
            sched_current()->exitcode = -EINVAL;
            sched_current()->state = TASK_ZOMBIE;

            // Go to scheduler
            sched_switch();
            // Scheduler mustn't schedule this task anymore
            panic_on_reach();
        }

        *status = tasks[child_pid].exitcode;
    }

    // Free task entry
    tasks[child_pid].state = TASK_NOT_ALLOCATED;
    printk("sys_wait: pid %d reaped child with pid %d\n", sched_current()->pid, child_pid);
    return 0;
}
