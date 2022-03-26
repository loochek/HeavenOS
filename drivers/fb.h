#ifndef FB_H
#define FB_H

#include "common.h"

typedef uint32_t fb_color_t;

#define FB_COLOR_BLACK ((fb_color_t)0)
#define FB_COLOR_WHITE ((fb_color_t)0x00FFFFFF)
#define FB_COLOR_RED   ((fb_color_t)0x00FF0000)
#define FB_COLOR_GREEN ((fb_color_t)0x0000FF00)
#define FB_COLOR_BLUE  ((fb_color_t)0x000000FF)

extern bool fb_initialized;

/**
 * Initializes framebuffer. 
 * Requires mb_fb_info to be valid, panics otherwise
 * 
 */
void fb_init();

/**
 * Returns framebuffer size
 *
 * \param width Where to store width
 * \param height Where to store height
 */
void fb_get_size(int *width, int *height);

/**
 * Clears framebuffer with specified color
 */
void fb_clear(fb_color_t color);

/**
 * Draws the filed rectangle
 * 
 * \param color Color
 * \param x X position
 * \param x Y position
 * \param width Rectangle width
 * \param height Rectangle height
 */
void fb_draw_rect(fb_color_t color, int x, int y, int width, int height);

/**
 * Draws the pixel array to the specified position. 
 * The pixel array must be a contiguous array of colors - 
 * rows from top to bottom, each row - colors from left to right
 * 
 * \param data Pixel array
 * \param x X position
 * \param x Y position
 * \param width Pixel array width
 * \param height Pixel array height
 */
void fb_draw(fb_color_t *data, int x, int y, int width, int height);

/**
 * Draws the 8x8 character glyph
 *
 * \param glyph Glyph data
 * \param x X position
 * \param x Y position
 * \param fg_color Foreground color
 * \param bg_color Background color
 */
void fb_draw_glyph(const uint8_t *glyph, int x, int y, fb_color_t fg_color, fb_color_t bg_color);

/**
 * Scrolls the content up
 * 
 * \param height Scroll height in pixels
 * \param color Color to fill scroll area with
 */
void fb_scroll_up(int height, fb_color_t color);

#endif
