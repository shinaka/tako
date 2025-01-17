#include "sprite_engine.h"
#include "hardware/dma.h"
#include <string.h>
#include "../pins.h"
#include "../externs.h"
#include "sprite_lookup.pio.h"
#include "sprite_pattern.pio.h"
#include "sprite_compose.pio.h"

static PIO engine_pio;
static uint engine_sm_lookup;
static uint engine_sm_pattern;
static uint engine_sm_compose;

static int dma_chan_pattern;
static int dma_chan_compose;

#define DISPLAY_HEIGHT 320
#define DISPLAY_WIDTH 480

static Sprite sprite_table[MAX_SPRITES];
static uint8_t sprites_per_line[DISPLAY_HEIGHT];
static uint8_t line_sprite_indices[DISPLAY_HEIGHT][MAX_SPRITES_PER_LINE];
static uint16_t palettes[SPRITE_PALETTES][COLORS_PER_PALETTE];

bool sprite_engine_init(PIO pio, uint sm_lookup, uint sm_pattern, uint sm_compose) 
{
    engine_pio = pio;
    engine_sm_lookup = sm_lookup;
    engine_sm_pattern = sm_pattern;
    engine_sm_compose = sm_compose;
    
    memset(sprite_table, 0, sizeof(sprite_table));
    memset(sprites_per_line, 0, sizeof(sprites_per_line));
    memset(palettes, 0, sizeof(palettes));
    
    uint offset_lookup = pio_add_program(pio, &sprite_lookup_program);
    uint offset_pattern = pio_add_program(pio, &sprite_pattern_program);
    uint offset_compose = pio_add_program(pio, &sprite_compose_program);
    
    pio_sm_config c_lookup = sprite_lookup_program_get_default_config(offset_lookup);
    sm_config_set_in_pins(&c_lookup, PIN_D0);  // For sprite table access
    pio_sm_init(pio, sm_lookup, offset_lookup, &c_lookup);
    
    pio_sm_config c_pattern = sprite_pattern_program_get_default_config(offset_pattern);
    sm_config_set_in_pins(&c_pattern, PIN_D0);  // For pattern data
    pio_sm_init(pio, sm_pattern, offset_pattern, &c_pattern);
    
    pio_sm_config c_compose = sprite_compose_program_get_default_config(offset_compose);
    sm_config_set_out_pins(&c_compose, PIN_D0, 1);  // For frame buffer output
    pio_sm_init(pio, sm_compose, offset_compose, &c_compose);
    
    dma_chan_pattern = dma_claim_unused_channel(true);
    dma_chan_compose = dma_claim_unused_channel(true);
    
    pio_sm_set_enabled(pio, sm_lookup, true);
    pio_sm_set_enabled(pio, sm_pattern, true);
    pio_sm_set_enabled(pio, sm_compose, true);
    
    return true;
}

bool sprite_update(uint8_t index, const Sprite* sprite) 
{
    if ((index >= MAX_SPRITES) ||
        (sprite->x >= DISPLAY_WIDTH || sprite->y >= DISPLAY_HEIGHT) ||
        ((sprite->attr & SPRITE_ATTR_SIZE_MASK) > SPRITE_SIZE_64x64))
        return false;
        
    sprite_table[index] = *sprite;

    return true;
}

bool pattern_load(uint16_t pattern_num, const uint8_t* data, uint8_t size) 
{
    if (pattern_num >= MAX_PATTERNS) return false;
    
    uint32_t pattern_size;

    switch(size) 
    {
        case SPRITE_SIZE_8x8:       // 8x8x4bpp = 256 bits = 32 bytes
            pattern_size = 32;   
            break;
        case SPRITE_SIZE_16x16:     // 16x16x4bpp = 1024 bits = 128 bytes
            pattern_size = 128;  
            break;
        case SPRITE_SIZE_32x32:     // 32x32x4bpp = 4096 bits = 512 bytes
            pattern_size = 512;  
            break;
        case SPRITE_SIZE_64x64:     // 64x64x4bpp = 16384 bits = 2048 bytes
            pattern_size = 2048; 
            break;
        default: 
            return false;
    }
    
    uint32_t addr = PSRAM_SPRITE_BASE + (pattern_num * 2048);
    
    return aps6404_write(&psram, addr, data, pattern_size);
}

bool palette_load(uint8_t palette_num, const uint16_t* colors) 
{
    if (palette_num >= SPRITE_PALETTES) 
        return false;
    
    memcpy(palettes[palette_num], colors, COLORS_PER_PALETTE * sizeof(uint16_t));

    return true;
}

void sprite_engine_start_frame(void) 
{
    memset(sprites_per_line, 0, sizeof(sprites_per_line));
    
    for (int i = 0; i < MAX_SPRITES; i++) 
    {
        if (!(sprite_table[i].ctrl & SPRITE_CTRL_ENABLE)) 
            continue;
        
        uint16_t sprite_height;
        switch(sprite_table[i].attr & SPRITE_ATTR_SIZE_MASK) 
        {
            case SPRITE_SIZE_8x8:
                sprite_height = 8;  
                break;
            case SPRITE_SIZE_16x16: 
                sprite_height = 16; 
                break;
            case SPRITE_SIZE_32x32: 
                sprite_height = 32; 
                break;
            case SPRITE_SIZE_64x64:
                sprite_height = 64; 
                break;
            default: 
                continue;
        }
        
        uint16_t start_line = sprite_table[i].y;
        uint16_t end_line = start_line + sprite_height;
        
        for (uint16_t line = start_line; line < end_line && line < DISPLAY_HEIGHT; line++) 
        {
            if (sprites_per_line[line] < MAX_SPRITES_PER_LINE) 
            {
                line_sprite_indices[line][sprites_per_line[line]++] = i;
            }
        }
    }
}

void sprite_engine_prepare_line(uint16_t line) 
{
    pio_sm_put_blocking(engine_pio, engine_sm_lookup, line);
    
    while(!pio_sm_is_rx_fifo_empty(engine_pio, engine_sm_compose)) 
    {
        pio_sm_get_blocking(engine_pio, engine_sm_compose);
    }
}

const Sprite *get_sprite_from_table(uint8_t index)
{
    if (index < MAX_SPRITES) 
    {
        return &sprite_table[index];
    }
    
    return NULL;
}
