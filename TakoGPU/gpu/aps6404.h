#pragma once
#include <stdint.h>
#include "hardware/pio.h"

// Memory Map
#define PSRAM_FRAME_BUFFER_BASE    0x000000    // Start of frame buffers
#define PSRAM_FRAME_BUFFER_SIZE    0x050000    // 320KB for frame buffers
#define PSRAM_SPRITE_BASE          0x050000    // Start of sprite patterns
#define PSRAM_SPRITE_SIZE          0x0B0000    // 704KB for sprites
#define PSRAM_TILEMAP_BASE         0x100000    // Start of tile maps
#define PSRAM_TILEMAP_SIZE         0x100000    // 1MB for tile maps

// APS6404L Commands (https://www.pjrc.com/store/APS6404L_3SQR.pdf)
#define APS6404_CMD_READ           0x03    // Read data
#define APS6404_CMD_FAST_READ      0x0B    // Fast read
#define APS6404_CMD_FAST_READ_QUAD 0xEB    // Quad fast read
#define APS6404_CMD_WRITE          0x02    // Write data
#define APS6404_CMD_WRITE_QUAD     0x38    // Quad write
#define APS6404_CMD_ENTER_QUAD     0x35    // Enter quad mode
#define APS6404_CMD_EXIT_QUAD      0xF5    // Exit quad mode
#define APS6404_CMD_RESET_ENABLE   0x66    // Reset enable
#define APS6404_CMD_RESET          0x99    // Reset device
#define APS6404_CMD_BURST_LENGTH   0xC0    // Set burst length

typedef struct {
    PIO pio;
    uint sm;
    uint offset;
    int dma_chan;
    uint cs_pin;
    uint sck_pin;
    uint data0_pin;
    uint data1_pin;
    uint data2_pin;
    uint data3_pin;
    bool quad_mode;
} APS6404State;

bool aps6404_init(APS6404State* psram, PIO pio, uint sm, uint sck, uint data0, uint data1, uint data2, uint data3, uint cs);
void aps6404_deinit(APS6404State* psram);

bool aps6404_write(APS6404State* psram, uint32_t addr, const uint8_t* data, size_t len);
bool aps6404_read(APS6404State* psram, uint32_t addr, uint8_t* data, size_t len);

typedef struct {
    bool passed;
    uint32_t failed_address;
    uint32_t expected;
    uint32_t received;
} MemTestResult;

MemTestResult aps6404_test(APS6404State* psram);