// Minimal remote framebuffer/input service.
//
// The protocol is intentionally small and unauthenticated: WPA2/WPA3 or an external SSH tunnel
// must provide network security. One client receives RGB222 RLE keyframes and may inject input.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include "input.h"
#include "os.h"
#include "remote.h"
#include "vga.h"

#define REMOTE_PORT 2323
#define PKT_FRAME 1
#define PKT_INPUT 2
#define INPUT_KEY 1
#define INPUT_MOUSE_ABS 2
#define INPUT_MOUSE_REL 3
#define INPUT_BUTTONS 4
#define INPUT_WHEEL 5
#define FRAME_MAX (VGA_W * VGA_H * 2 + 16)

static volatile bool s_frame_ready;
static int s_client = -1;
static uint8_t s_rx[40];
static size_t s_rx_n;

static bool send_all(int fd, const void *data, size_t n)
{
    const uint8_t *p = data;
    while (n) {
        int r = send(fd, p, n, 0);
        if (r <= 0) return false;
        p += r;
        n -= (size_t)r;
    }
    return true;
}

static bool send_frame(int fd)
{
    uint8_t *copy = heap_caps_malloc(VGA_W * VGA_H, MALLOC_CAP_SPIRAM);
    uint8_t *rle = heap_caps_malloc(FRAME_MAX, MALLOC_CAP_SPIRAM);
    if (!copy || !rle) { free(copy); free(rle); return false; }
    memcpy(copy, vga_fb(), VGA_W * VGA_H);
    vga_cursor_overlay(copy);
    size_t n = 0;
    for (int i = 0; i < VGA_W * VGA_H;) {
        int run = 1;
        while (i + run < VGA_W * VGA_H && run < 255 && copy[i + run] == copy[i]) run++;
        if (n + 2 > FRAME_MAX) { free(copy); free(rle); return false; }
        rle[n++] = (uint8_t)run;
        rle[n++] = copy[i];
        i += run;
    }
    uint8_t h[12] = {
        'B', 'V', 1, PKT_FRAME,
        (uint8_t)(VGA_W & 255), (uint8_t)(VGA_W >> 8),
        (uint8_t)(VGA_H & 255), (uint8_t)(VGA_H >> 8),
        (uint8_t)(n & 255), (uint8_t)(n >> 8), (uint8_t)(n >> 16), (uint8_t)(n >> 24)
    };
    bool ok = send_all(fd, h, sizeof(h)) && send_all(fd, rle, n);
    free(copy); free(rle);
    return ok;
}

static void handle_input(const uint8_t *p, uint16_t n)
{
    if (n < 1) return;
    switch (p[0]) {
    case INPUT_KEY:
        if (n >= 3) input_key(p[1], p[2] != 0);
        break;
    case INPUT_MOUSE_ABS:
        if (n >= 5) input_mouse_abs((int)(p[1] | p[2] << 8), (int)(p[3] | p[4] << 8));
        break;
    case INPUT_MOUSE_REL:
        if (n >= 5) input_mouse_rel((int16_t)(p[1] | p[2] << 8), (int16_t)(p[3] | p[4] << 8));
        break;
    case INPUT_BUTTONS:
        if (n >= 2) input_mouse_buttons(p[1]);
        break;
    case INPUT_WHEEL:
        if (n >= 3) input_wheel((int16_t)(p[1] | p[2] << 8));
        break;
    }
}

static void remote_task(void *arg)
{
    (void)arg;
    int listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_fd < 0) { vTaskDelete(NULL); return; }
    int one = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(REMOTE_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(listen_fd, 1) < 0) {
        close(listen_fd); vTaskDelete(NULL); return;
    }
    for (;;) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listen_fd, &readfds);
        int maxfd = listen_fd;
        if (s_client >= 0) { FD_SET(s_client, &readfds); if (s_client > maxfd) maxfd = s_client; }
        struct timeval tv = {.tv_sec = 0, .tv_usec = 100000};
        select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (FD_ISSET(listen_fd, &readfds)) {
            int fd = accept(listen_fd, NULL, NULL);
            if (fd >= 0) {
                if (s_client >= 0) close(s_client);
                s_client = fd;
                s_rx_n = 0;
                s_frame_ready = true;
            }
        }
        if (s_client >= 0 && FD_ISSET(s_client, &readfds)) {
            int n = recv(s_client, s_rx + s_rx_n, sizeof(s_rx) - s_rx_n, MSG_DONTWAIT);
            if (n <= 0) { close(s_client); s_client = -1; s_rx_n = 0; }
            else {
                s_rx_n += (size_t)n;
                while (s_rx_n >= 6) {
                    if (s_rx[0] != 'B' || s_rx[1] != 'V' || s_rx[2] != 1 || s_rx[3] != PKT_INPUT) {
                        close(s_client); s_client = -1; s_rx_n = 0; break;
                    }
                    uint16_t len = s_rx[4] | s_rx[5] << 8;
                    if (len > 32) { close(s_client); s_client = -1; s_rx_n = 0; break; }
                    if (s_rx_n < (size_t)len + 6) break;
                    handle_input(s_rx + 6, len);
                    memmove(s_rx, s_rx + 6 + len, s_rx_n - 6 - len);
                    s_rx_n -= 6 + len;
                }
            }
        }
        if (s_client >= 0 && s_frame_ready) {
            s_frame_ready = false;
            if (!send_frame(s_client)) { close(s_client); s_client = -1; }
        }
    }
}

void remote_start(void)
{
    static bool started;
    if (!started) {
        started = true;
        net_start();
        xTaskCreatePinnedToCore(remote_task, "remote", 6144, NULL, 3, NULL, SYS_CORE);
    }
}

void remote_frame_ready(void) { s_frame_ready = true; }
