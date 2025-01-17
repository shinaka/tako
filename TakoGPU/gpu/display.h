#pragma once

#include <stdint.h>
#include "hardware/pio.h"

#define DISPLAY_WIDTH    320
#define DISPLAY_HEIGHT   240

// 0=0deg, 1=90deg, 2=180deg, 3=270deg
#define DISPLAY_ROTATION 0      

// ST7789 cmds that might not be right
#define DISP_CMD_NOP         0x00
#define DISP_CMD_SWRESET     0x01
#define DISP_CMD_SLPIN       0x10
#define DISP_CMD_SLPOUT      0x11
#define DISP_CMD_PTLON       0x12
#define DISP_CMD_NORON       0x13
#define DISP_CMD_INVOFF      0x20
#define DISP_CMD_INVON       0x21
#define DISP_CMD_GAMSET      0x26
#define DISP_CMD_DISPOFF     0x28
#define DISP_CMD_DISPON      0x29
#define DISP_CMD_CASET       0x2A
#define DISP_CMD_RASET       0x2B
#define DISP_CMD_RAMWR       0x2C
#define DISP_CMD_RAMRD       0x2E
#define DISP_CMD_MADCTL      0x36
#define DISP_CMD_COLMOD      0x3A
#define DISP_CMD_PIXSET      0x3A

bool display_init(PIO pio, uint sm);
void display_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void display_write_cmd(uint8_t cmd);
void display_write_data(uint8_t data);
void display_write_pixel(uint16_t color);
void display_start_pixels(void);

void display_swap_buffers(void);
void display_wait_for_frame_complete(void);
uint16_t* display_get_next_buffer(void);
