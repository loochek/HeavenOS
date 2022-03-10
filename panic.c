#include "panic.h"
#include "console.h"
#include "printk.h"

void __panic(const char* location, const char* fmt, ...)
{
    if (cons_initialized)
    {
        printk_color(FB_COLOR_WHITE, FB_COLOR_RED, "Kernel panic at %s: ", location);

        va_list args;
        va_start(args, fmt);
        vprintk_color(fmt, args, FB_COLOR_WHITE, FB_COLOR_RED);
        va_end(args);

        printk("\n");
    }

    while (true)
        asm volatile("hlt");
}
