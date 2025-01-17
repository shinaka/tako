#include "display.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include <string.h>
#include "display_spi.pio.h"
#include "../pins.h"
#include <stdlib.h>
#include "pico/stdlib.h"

static PIO display_pio;
static uint display_sm;
static int dma_chan;
static volatile bool frame_in_progress;
static uint16_t* frame_buffers[2];
static int current_buffer = 0;
static bool initialized = false;

bool display_init(PIO pio, uint sm) 
{
    if (initialized)
        return false;
    
    display_pio = pio;
    display_sm = sm;
    
    gpio_init(PIN_DISP_CS);
    gpio_init(PIN_DISP_DC);
    gpio_init(PIN_DISP_RST);
    gpio_init(PIN_DISP_BL);
    
    gpio_set_dir(PIN_DISP_CS, GPIO_OUT);
    gpio_set_dir(PIN_DISP_DC, GPIO_OUT);
    gpio_set_dir(PIN_DISP_RST, GPIO_OUT);
    gpio_set_dir(PIN_DISP_BL, GPIO_OUT);
    
    gpio_put(PIN_DISP_CS, 1); // deselect display
    gpio_put(PIN_DISP_DC, 1); // data mode
    gpio_put(PIN_DISP_BL, 0); // backlight off
    
    uint offset = pio_add_program(pio, &display_spi_program);
    pio_sm_config c = display_spi_program_get_default_config(offset);
    
    sm_config_set_out_pins(&c, PIN_DISP_MOSI, 1);
    sm_config_set_sideset_pins(&c, PIN_DISP_SCK);
    
    // spi (system clock / 2)
    float div = clock_get_hz(clk_sys) / (133000000 * 2);
    sm_config_set_clkdiv(&c, div);
        
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
    
    dma_chan = dma_claim_unused_channel(true);
    
    // framebuffers
    frame_buffers[0] = malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);
    frame_buffers[1] = malloc(DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);
    
    if (!frame_buffers[0] || !frame_buffers[1]) {
        return false;
    }
    
    // Hardware reset
    gpio_put(PIN_DISP_RST, 0);
    sleep_ms(100);
    gpio_put(PIN_DISP_RST, 1);
    sleep_ms(100);
    
    // Initialize display
    display_write_cmd(DISP_CMD_SWRESET);
    sleep_ms(150);
    
    display_write_cmd(DISP_CMD_SLPOUT); // no sleep
    sleep_ms(500);
    
    display_write_cmd(DISP_CMD_COLMOD); // color mode
    display_write_data(0x55); // 16 bit color
    
    display_write_cmd(DISP_CMD_MADCTL);
    display_write_data(DISPLAY_ROTATION << 5); // rotation (i think this is wrong?)
    
    display_write_cmd(DISP_CMD_INVON); // display inversion
    
    display_write_cmd(DISP_CMD_NORON); // normal display mode on
    
    display_write_cmd(DISP_CMD_DISPON); // screen turn on
    sleep_ms(50);
    
    gpio_put(PIN_DISP_BL, 1); // backlight on
    
    initialized = true;
    frame_in_progress = false;
    
    return true;
}

void display_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) 
{
    display_write_cmd(DISP_CMD_CASET);
    display_write_data(x1 >> 8);
    display_write_data(x1 & 0xFF);
    display_write_data(x2 >> 8);
    display_write_data(x2 & 0xFF);
    
    display_write_cmd(DISP_CMD_RASET);
    display_write_data(y1 >> 8);
    display_write_data(y1 & 0xFF);
    display_write_data(y2 >> 8);
    display_write_data(y2 & 0xFF);
}

void display_write_cmd(uint8_t cmd) 
{
    gpio_put(PIN_DISP_DC, 0);
    gpio_put(PIN_DISP_CS, 0);
    
    pio_sm_put_blocking(display_pio, display_sm, cmd);
    
    gpio_put(PIN_DISP_CS, 1);
}

void display_write_data(uint8_t data) 
{
    gpio_put(PIN_DISP_DC, 1);
    gpio_put(PIN_DISP_CS, 0);
    
    pio_sm_put_blocking(display_pio, display_sm, data);
    
    gpio_put(PIN_DISP_CS, 1);
}

void display_write_pixel(uint16_t color) 
{
    gpio_put(PIN_DISP_DC, 1);
    gpio_put(PIN_DISP_CS, 0);
    
    pio_sm_put_blocking(display_pio, display_sm, color >> 8);
    pio_sm_put_blocking(display_pio, display_sm, color & 0xFF);
    
    gpio_put(PIN_DISP_CS, 1);
}

void display_start_pixels(void) 
{
    display_write_cmd(DISP_CMD_RAMWR);
    gpio_put(PIN_DISP_DC, 1);
    gpio_put(PIN_DISP_CS, 0);
}

void display_swap_buffers(void) 
{
    if (frame_in_progress) return;
    
    frame_in_progress = true;
    
    display_set_window(0, 0, DISPLAY_WIDTH-1, DISPLAY_HEIGHT-1);
    display_start_pixels();

    dma_channel_config config = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, pio_get_dreq(display_pio, display_sm, true));
    
    dma_channel_configure(dma_chan, &config, &display_pio->txf[display_sm], frame_buffers[current_buffer], DISPLAY_WIDTH * DISPLAY_HEIGHT, true);

    current_buffer = !current_buffer;
}

void display_wait_for_frame_complete(void) 
{
    if (!frame_in_progress)
        return;
    
    dma_channel_wait_for_finish_blocking(dma_chan);
    gpio_put(PIN_DISP_CS, 1);
    frame_in_progress = false;
}

uint16_t* display_get_next_buffer(void) 
{
    return frame_buffers[current_buffer];
}