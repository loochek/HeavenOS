#ifndef CONSOLE_H
#define CONSOLE_H

#include "drivers/fb.h"

extern bool cons_initialized;

/**
 * Initializes the console
 */
void cons_init();

void cons_putchar_color(char c, fb_color_t fg_color, fb_color_t bg_color);
void cons_putchar(char c);

void cons_print_color(const char *str, fb_color_t fg_color, fb_color_t bg_color);
void cons_print(const char *str);

void cons_write_color(const char *data, int size, fb_color_t fg_color, fb_color_t bg_color);
void cons_write(const char *data, int size);

#endif
