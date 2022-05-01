#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define syscall_arg0(regs) ((regs)->rdi)
#define syscall_arg1(regs) ((regs)->rsi)
#define syscall_arg2(regs) ((regs)->rdx)
#define syscall_arg3(regs) ((regs)->r8)
#define syscall_arg4(regs) ((regs)->r9)
#define syscall_arg5(regs) ((regs)->r10)

#endif
