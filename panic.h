#ifndef PANIC_H
#define PANIC_H

#include "printk.h"

#define __S1(x) #x
#define __S2(x) __S1(x)

void __panic(const char* location, const char* fmt, ...);

#define panic(msg, ...) __panic(__FILE__ ":" __S2(__LINE__), msg __VA_OPT__(,) __VA_ARGS__)

/**
 * Checks expression to be true, panics otherwise. 
 * Use it for vital kernel checks
 */
#define kassert(expr) if (!(expr)) { panic("assertion failed: '" #expr "'"); }

#ifndef NDEBUG
// Same as kassert, but enabled only while debugging
#define kassert_dbg(expr) kassert(expr)
#else
#define kassert_dbg(expr)
#endif

#endif
