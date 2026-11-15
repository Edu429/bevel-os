// Headless 640x480 RGB222 framebuffer, one byte per pixel, in PSRAM.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define VGA_W 640
#define VGA_H 480
#define VGA_ABS_SPAN_X 678   // H total 848 * 0.8: width of a standard-timing capture in our pixels
#define CUR_MAX_W 16
#define CUR_MAX_H 20

// RGB222 pixel: bits [1:0]=R, [3:2]=G, [5:4]=B, each 0..3
#define RGB222(r, g, b) ((uint8_t)((((b) & 3) << 4) | (((g) & 3) << 2) | ((r) & 3)))

// Starts the panel. The scan-out interrupt runs on the core that calls this.
esp_err_t vga_init(void);
uint8_t *vga_fb(void);

// Mouse pointer sprite. px = w*h bytes, 0xFF = transparent. Max 16x20.
void vga_cursor_shape(const uint8_t *px, int w, int h, int hot_x, int hot_y);
void vga_cursor_move(int x, int y);
void vga_cursor_show(bool on);
void vga_cursor_get(int *x, int *y, bool *shown);
void vga_cursor_overlay(uint8_t *dst_fb_copy); // draw the cursor into a copy of the frame (for dumps)

// Beam racing: block until rows [y0,y1) can be rewritten without the scan-out reading them
// half-way, assuming the write takes est_us. Returns the time waited in microseconds.
int vga_wait_rows(int y0, int y1, int est_us);
// Next framebuffer row the scan-out will fetch.
int vga_fetch_row(void);

typedef struct {
    uint32_t frames;        // frames scanned out
    uint32_t fills;         // retained for serial compatibility
    uint32_t isr_avg_us10;  // retained for serial compatibility
    uint32_t isr_max_us;    // retained for serial compatibility
    int32_t slack_min_us;   // retained for serial compatibility
    uint32_t late;          // retained for serial compatibility
    uint32_t isr_core;      // retained for serial compatibility
} vga_stats_t;
void vga_get_stats(vga_stats_t *out, bool reset);

// 0 = libc memcpy, 1 = PIE 128-bit copy
void vga_set_copy_mode(int mode);
int vga_get_copy_mode(void);
