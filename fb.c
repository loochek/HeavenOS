#include "panic.h"
#include "multiboot.h"
#include "fb.h"

bool fb_initialized = false;

static uint8_t *fb_addr = NULL;

static int fb_width    = 0;
static int fb_height   = 0;
static int fb_pitch    = 0;
static int fb_buf_size = 0;

static void fb_put_line_fragment(fb_color_t *data, int x, int y, int width);
static void fb_put_rect_fragment(fb_color_t color, int x, int y, int width);

void fb_init()
{
    kassert(mb_fb_info != NULL && mb_fb_info->type == 1 || mb_fb_info->depth == 32);

    fb_width  = mb_fb_info->width;
    fb_height = mb_fb_info->heigth;
    fb_addr   = mb_fb_info->addr;
    fb_pitch  = mb_fb_info->pitch;

    fb_buf_size = mb_fb_info->pitch * fb_height;

    fb_initialized = true;
}

void fb_get_size(int *width, int *height)
{
    kassert(fb_initialized);
    kassert_dbg(width != NULL && height != NULL);

    *width = fb_width;
    *height = fb_height;
}

void fb_clear(fb_color_t color)
{
    kassert(fb_initialized);

    for (int y = 0; y < fb_height; y++)
    {
        fb_color_t *row_addr = (fb_color_t*)(fb_addr + y * fb_pitch);
        for (int x = 0; x < fb_width; x++)
            row_addr[x] = color;
    }
}

void fb_draw_rect(fb_color_t color, int x, int y, int width, int height)
{
    kassert(fb_initialized);

    for (int i = 0; i < height; i++)
    {
        fb_put_rect_fragment(color, x, y + i, width);
    }
}

void fb_draw(fb_color_t *data, int x, int y, int width, int height)
{
    kassert(fb_initialized);
    kassert_dbg(data != NULL);
    
    for (int i = 0; i < height; i++)
    {
        fb_put_line_fragment(data + (width * i), x, y + i, width);
    }
}

void fb_draw_glyph(const uint8_t *glyph, int x, int y, fb_color_t fg_color, fb_color_t bg_color)
{
    kassert(fb_initialized);
    kassert_dbg(glyph != NULL);

    for (int h = 0; h < 8; h++)
    {
        fb_color_t *addr = (fb_color_t*)(fb_addr + (y + h) * fb_pitch + x * sizeof(fb_color_t));
        for (int w = 0; w < 8; w++)
        {
            bool pixel_set = (glyph[h] >> w) & 1;
            addr[w] = pixel_set ? fg_color : bg_color;
        }
    }
}

void fb_scroll_up(int height, fb_color_t color)
{
    kassert(fb_initialized);

    memmove(fb_addr, fb_addr + fb_pitch * height, fb_pitch * (fb_height - height) * sizeof(fb_color_t));
    for (int y = fb_height - height; y < fb_height; y++)
        fb_put_rect_fragment(color, 0, y, fb_width);
}

static void fb_put_line_fragment(fb_color_t *data, int x, int y, int width)
{
    kassert_dbg(data != NULL);

    if (y < 0 || y >= fb_height)
        return;

    if (x + width > fb_width)
        width = fb_width - x;

    fb_color_t *addr = (fb_color_t*)(fb_addr + y * fb_pitch + x * sizeof(fb_color_t));
    memcpy(addr, data, sizeof(fb_color_t) * width);
}

static void fb_put_rect_fragment(fb_color_t color, int x, int y, int width)
{
    if (y < 0 || y >= fb_height)
        return;

    if (x + width > fb_width)
        width = fb_width - x;

    fb_color_t *addr = (fb_color_t*)(fb_addr + y * fb_pitch + x * sizeof(fb_color_t));
    for (int i = 0; i < width; i++)
        addr[i] = color;
}
