// Headless framebuffer backend.
//
// The desktop keeps its 640x480 RGB222 surface and cursor semantics, but this backend no longer
// starts an LCD/VGA peripheral. A remote display service can consume the framebuffer over Wi-Fi.
#include <string.h>
#include "vga.h"
#include "esp_heap_caps.h"

static uint8_t *s_fb;
static uint8_t s_cur_px[CUR_MAX_W * CUR_MAX_H];
static int16_t s_cur_x, s_cur_y;
static int8_t s_cur_w, s_cur_h, s_cur_hx, s_cur_hy;
static bool s_cur_on;

esp_err_t vga_init(void)
{
    if (s_fb) return ESP_OK;
    s_fb = heap_caps_aligned_calloc(64, 1, VGA_W * VGA_H, MALLOC_CAP_SPIRAM);
    return s_fb ? ESP_OK : ESP_ERR_NO_MEM;
}

uint8_t *vga_fb(void) { return s_fb; }

void vga_cursor_shape(const uint8_t *px, int w, int h, int hot_x, int hot_y)
{
    if (w > CUR_MAX_W) w = CUR_MAX_W;
    if (h > CUR_MAX_H) h = CUR_MAX_H;
    int px_x = s_cur_x + s_cur_hx, px_y = s_cur_y + s_cur_hy;
    bool was = s_cur_on;
    s_cur_on = false;
    memset(s_cur_px, 0xFF, sizeof(s_cur_px));
    for (int y = 0; y < h; y++) memcpy(s_cur_px + y * CUR_MAX_W, px + y * w, w);
    s_cur_w = w; s_cur_h = h; s_cur_hx = hot_x; s_cur_hy = hot_y;
    s_cur_x = px_x - hot_x; s_cur_y = px_y - hot_y;
    s_cur_on = was;
}

void vga_cursor_move(int x, int y)
{
    s_cur_x = x - s_cur_hx;
    s_cur_y = y - s_cur_hy;
}

void vga_cursor_show(bool on) { s_cur_on = on; }

void vga_cursor_get(int *x, int *y, bool *shown)
{
    if (x) *x = s_cur_x + s_cur_hx;
    if (y) *y = s_cur_y + s_cur_hy;
    if (shown) *shown = s_cur_on;
}

void vga_cursor_overlay(uint8_t *dst)
{
    if (!s_cur_on) return;
    for (int y = 0; y < s_cur_h; y++) {
        for (int x = 0; x < s_cur_w; x++) {
            int sx = s_cur_x + x, sy = s_cur_y + y;
            uint8_t p = s_cur_px[y * CUR_MAX_W + x];
            if (p != 0xFF && sx >= 0 && sx < VGA_W && sy >= 0 && sy < VGA_H) dst[sy * VGA_W + sx] = p;
        }
    }
}

int vga_fetch_row(void) { return 0; }
int vga_wait_rows(int y0, int y1, int est_us) { (void)y0; (void)y1; (void)est_us; return 0; }

void vga_get_stats(vga_stats_t *o, bool reset)
{
    (void)reset;
    memset(o, 0, sizeof(*o));
}

void vga_set_copy_mode(int mode) { (void)mode; }
int vga_get_copy_mode(void) { return 0; }
