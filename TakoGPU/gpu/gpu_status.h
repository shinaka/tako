#pragma once
#include <stdint.h>

// status codes
typedef enum {
    GPU_STATUS_OK = 0,
    GPU_STATUS_BUSY = 1,
    GPU_STATUS_ERROR = 2,
    GPU_STATUS_INVALID_COMMAND = 3,
    GPU_STATUS_BUFFER_FULL = 4,
    GPU_STATUS_INVALID_PARAMETER = 5
} GpuStatusCode;

// error codes
typedef enum {
    GPU_ERROR_NONE = 0,
    GPU_ERROR_INVALID_SPRITE = 1,
    GPU_ERROR_INVALID_PATTERN = 2,
    GPU_ERROR_INVALID_PALETTE = 3,
    GPU_ERROR_MEMORY_FULL = 4,
    GPU_ERROR_TRANSFER_FAILED = 5,
    GPU_ERROR_TIMEOUT = 6,
    GPU_ERROR_NOT_INITIALIZED = 7
} GpuErrorCode;

// busy flags
#define GPU_BUSY_SPRITE_UPDATE    0x01
#define GPU_BUSY_PATTERN_LOAD     0x02
#define GPU_BUSY_PALETTE_UPDATE   0x04
#define GPU_BUSY_DISPLAY_REFRESH  0x08
#define GPU_BUSY_DMA_TRANSFER     0x10
#define GPU_BUSY_COMMAND_QUEUE    0x20

// status response
typedef struct __attribute__((packed)) {
    uint8_t status;        // GpuStatusCode
    uint8_t busy;          // busy flags
    uint8_t error_code;    // GpuErrorCode if status is GPU_STATUS_ERROR
    uint8_t sprite_count;  // number of active sprites
    uint8_t frame_rate;
    uint8_t reserved[3];
} GpuStatus;

GpuStatus gpu_get_status(void);

void gpu_clear_error(void);
