#pragma once

#include <stdint.h>
#include "hardware/pio.h"
#include "aps6404.h"

#define MAX_PATTERNS 1024

#define MAX_SPRITES 128
#define MAX_SPRITES_PER_LINE 32
#define SPRITE_PALETTES 16
#define COLORS_PER_PALETTE 16

#define SPRITE_SIZE_8x8     0
#define SPRITE_SIZE_16x16   1
#define SPRITE_SIZE_32x32   2
#define SPRITE_SIZE_64x64   3

#define SPRITE_ATTR_SIZE_MASK   0x03
#define SPRITE_ATTR_HFLIP       0x04
#define SPRITE_ATTR_VFLIP       0x08
#define SPRITE_ATTR_PALETTE     0x70
#define SPRITE_ATTR_PRIORITY    0x80

#define SPRITE_CTRL_ENABLE      0x01
#define SPRITE_CTRL_TRANS       0x02

typedef struct __attribute__((packed)) {
    uint16_t x;
    uint16_t y;
    uint16_t pattern;
    uint8_t attr;
    uint8_t ctrl;
} Sprite;

bool sprite_engine_init(PIO pio, uint sm_lookup, uint sm_pattern, uint sm_compose);

bool sprite_update(uint8_t index, const Sprite* sprite);
bool sprite_enable(uint8_t index);
bool sprite_disable(uint8_t index);

bool pattern_load(uint16_t pattern_num, const uint8_t* data, uint8_t size);
bool palette_load(uint8_t palette_num, const uint16_t* colors);

void sprite_engine_start_frame(void);
void sprite_engine_prepare_line(uint16_t line);

const Sprite *get_sprite_from_table(uint8_t index);