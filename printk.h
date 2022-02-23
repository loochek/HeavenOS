#ifndef PRINTK_H
#define PRINTK_H

#include "common.h"
#include "fb.h"

// Original implementation by @pixelindigo: https://github.com/carzil/KeltOS/blob/master/kernel/printk.c

void printk_color(fb_color_t fg_color, fb_color_t bg_color, const char* fmt, ...);
void printk(const char* fmt, ...);

void vprintk_color(const char* fmt, va_list args, fb_color_t fg_color, fb_color_t bg_color);
void vprintk(const char* fmt, va_list args);

#endif