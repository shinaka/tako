#pragma once

#include <stdint.h>
#include "hardware/pio.h"
#include "hardware/gpio.h"

#define CMD_FLAG_NEEDS_RESPONSE   0x80 // bit 7: requires response
#define CMD_FLAG_HIGH_PRIORITY    0x40 // bit 6: priority command
#define CMD_FLAG_RESET_STATE      0x20 // bit 5: reset state before command

typedef enum {
    CMD_NOP             = 0x00,
    CMD_INIT            = 0x01,
    CMD_LOAD_PATTERN    = 0x02,
    CMD_LOAD_PALETTE    = 0x03,
    CMD_UPDATE_SPRITE   = 0x04,
    CMD_ENABLE_SPRITE   = 0x05,
    CMD_DISABLE_SPRITE  = 0x06,
    CMD_SET_SCROLL      = 0x07,
    CMD_STATUS          = 0x08,
    CMD_RESET           = 0xFF
} GpuCommand;

typedef struct __attribute__((packed)) {
    uint8_t cmd; // command type
    uint8_t flags; // command flags (above)
} GpuCommandHeader;

typedef struct {
    uint16_t pattern_num;
    uint8_t size; // SPRITE_SIZE_8x8, SPRITE_SIZE_16x16, etc.
    // data is in command buffer
} LoadPatternData;

typedef struct __attribute__((packed)) {
    uint8_t palette_num; 
    // 16 colors (32 bytes of RGB565) in buffer
} LoadPaletteData;

typedef struct __attribute__((packed)) {
    uint8_t sprite_num;
    uint16_t x;
    uint16_t y;
    uint8_t pattern;
    uint8_t attr; 
    uint8_t ctrl;
} UpdateSpriteData;

typedef struct __attribute__((packed)) {
    uint16_t x;
    uint16_t y;
    uint8_t layer;
} SetScrollData;

// Transfer state management
typedef struct {
    PIO pio;
    uint sm;
    uint offset;
    volatile bool transfer_active;
    volatile bool waiting_for_response;
} TransferState;

// helper functions for flag handling
static inline bool cmd_needs_response(const GpuCommandHeader* header) 
{
    return (header->flags & CMD_FLAG_NEEDS_RESPONSE) != 0;
}

static inline bool cmd_is_high_priority(const GpuCommandHeader* header) 
{
    return (header->flags & CMD_FLAG_HIGH_PRIORITY) != 0;
}

static inline bool cmd_resets_state(const GpuCommandHeader* header) 
{
    return (header->flags & CMD_FLAG_RESET_STATE) != 0;
}

bool transfer_init(TransferState* state, PIO pio, uint sm);

bool transfer_send_data(TransferState* state, const void* data, size_t len);
bool transfer_receive_data(TransferState* state, void* data, size_t len);
bool transfer_send_response(TransferState* state, const void* data, size_t len);
bool transfer_wait_response(TransferState* state, void* data, size_t len);