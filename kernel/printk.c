#include "kernel/panic.h"
#include "kernel/printk.h"
#include "kernel/console.h"

enum { PRINTK_BUF_SIZE = 11 };
static const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static size_t bprintu64(char* buf, uint64_t a, int base)
{
    size_t i;
    size_t bytes;

    if (a == 0)
    {
        buf[0] = '0';
        return 1;
    }
    for (bytes = 0; a > 0; bytes++)
    {
        buf[bytes] = digits[a % base];
        a /= base;
    }
    for (i = 0; i < bytes / 2; i++)
    {
        buf[i] ^= buf[bytes - i - 1];
        buf[bytes - i - 1] ^= buf[i];
        buf[i] ^= buf[bytes - i - 1];
    }
    return bytes;
}

static size_t bprints64(char* buf, int64_t a, int base)
{
    if (a < 0)
    {
        *(buf++) = '-';
        return 1 + bprintu64(buf, -a, base);
    }
    return bprintu64(buf, a, base);
}

static size_t bprintstr(char* buf, const char* str)
{
    const char* cur = str;
    size_t length = 0;
    while (*cur)
    {
        buf[length++] = *(cur++);
    }
    return length;
}

static size_t bprintptr(char* buf, void* ptr)
{
    size_t bytes;
    if (ptr == NULL)
    {
        return bprintstr(buf, "(nil)");
    }
    bytes = bprintstr(buf, "0x");
    return bytes + bprintu64(buf + bytes, (uint64_t) ptr, 16);
}

void vprintk_color(const char* fmt, va_list args, fb_color_t fg_color, fb_color_t bg_color)
{
    kassert(cons_initialized);
    
    const char* cursor = fmt;
    int idle = 1;
    uint32_t u32value;
    int32_t s32value;
    void* ptrvalue;
    char* str;
    char buf[PRINTK_BUF_SIZE];
    size_t size;

    while (*cursor) {
        if (idle) {
            if (*cursor != '%') {
                cons_write_color(cursor, 1, fg_color, bg_color);
            } else {
                idle = 0;
            }
        } else {
            switch (*cursor) {
            case 'u':
                u32value = va_arg(args, uint32_t);
                size = bprintu64(buf, u32value, 10);
                break;
            case 'x':
                u32value = va_arg(args, uint32_t);
                size = bprintu64(buf, u32value, 16);
                break;
            case 'd':
                s32value = va_arg(args, int32_t);
                size = bprints64(buf, s32value, 10);
                break;
            case 'p':
                ptrvalue = va_arg(args, void*);
                size = bprintptr(buf, ptrvalue);
                break;
            case 's':
                str = va_arg(args, char*);
                cons_print_color(str, fg_color, bg_color);
                size = 0;
                break;
            default:
                size = 0;
                break;
            }
            if (size) {
                cons_write_color(buf, size, fg_color, bg_color);
            }
            idle = 1;
        }
        ++cursor;
    }
}

void vprintk(const char* fmt, va_list args)
{
    kassert(cons_initialized);
    vprintk_color(fmt, args, FB_COLOR_WHITE, FB_COLOR_BLACK);
}

void printk_color(fb_color_t fg_color, fb_color_t bg_color, const char* fmt, ...)
{
    kassert(cons_initialized);

    va_list args;
    va_start(args, fmt);
    vprintk_color(fmt, args, fg_color, bg_color);
    va_end(args);
}

void printk(const char* fmt, ...)
{
    kassert(cons_initialized);

    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);
}
