// Shared declarations for Bevel OS.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "gfx.h"
#include "theme.h"

#define OS_NAME "Bevel OS"
#define OS_VERSION "1.0"

// Which core runs what. The GUI task and framebuffer live on GUI_CORE; USB, Wi-Fi, serial and
// loaded apps live on the other one.
#define GUI_CORE 1
#define SYS_CORE 0

// serial.c
void serial_start(void);
void serial_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
bool os_serial_command(const char *cmd, char *arg); // extra commands; false = unknown

// perf.c
typedef struct { char name[28]; uint32_t value; const char *unit; } perf_result_t;
#define PERF_MAX 48
extern perf_result_t perf_results[PERF_MAX];
extern int perf_count;
void perf_run(const char *which);   // call in the GUI task; "all" or a group name
uint32_t os_uptime_ms(void);

// Hooks filled in by later subsystems (SD card, loaded apps)
void os_sd_summary(char *buf, int n);       // "1.9 GB free of 3.7 GB" / "no card"
int  os_task_mem_kb(int task_id);           // -1 for system windows
bool os_task_kill(int task_id);             // false if task_id is not a loaded app

// storage.c
#define DISK_ROOT "/disk"
bool storage_init(void);
bool storage_ok(void);
bool storage_df(uint64_t *total, uint64_t *free_bytes);
void storage_unmount(void);
void os_fmt_size(char *buf, int n, uint64_t bytes);

// Grow a PSRAM block. Unlike realloc, the copy happens outside the allocator's lock, so a large
// block never stalls interrupts (the display's scan-out) while it is moved.
void *os_grow(void *old, size_t old_bytes, size_t new_bytes);
