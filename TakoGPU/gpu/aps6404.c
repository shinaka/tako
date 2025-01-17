// aps6404.c
#include "aps6404.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include <string.h>
#include "aps6404_quad.pio.h"
#include "pico/stdlib.h"

// if this doesn't work just adapt https://github.com/polpo/rp2040-psram/blob/main/psram_spi.h

bool aps6404_init(APS6404State* psram, PIO pio, uint sm, uint sck, uint data0, uint data1, uint data2, uint data3, uint cs) 
{
    psram->pio = pio;
    psram->sm = sm;
    psram->cs_pin = cs;
    psram->sck_pin = sck;
    psram->data0_pin = data0;
    psram->data1_pin = data1;
    psram->data2_pin = data2;
    psram->data3_pin = data3;
    psram->quad_mode = false;

    gpio_init(cs);
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs, 1); // deselect PSRAM

    pio_gpio_init(pio, sck);
    pio_gpio_init(pio, data0);
    pio_gpio_init(pio, data1);
    pio_gpio_init(pio, data2);
    pio_gpio_init(pio, data3);

    psram->offset = pio_add_program(pio, &aps6404_quad_program);
    pio_sm_config c = aps6404_quad_program_get_default_config(psram->offset);
    
    sm_config_set_out_pins(&c, data0, 4);
    sm_config_set_sideset_pins(&c, sck);
    
    // clock 133MHz
    float div = clock_get_hz(clk_sys) / (133.0f * 1000 * 1000);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, psram->offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    // I think timing is important here for the pio to handle, but i'll try this first!
    gpio_put(cs, 0);
    uint8_t reset_enable = APS6404_CMD_RESET_ENABLE;
    pio_sm_put_blocking(pio, sm, reset_enable);
    gpio_put(cs, 1);

    sleep_us(10);

    gpio_put(cs, 0);
    uint8_t reset = APS6404_CMD_RESET;
    pio_sm_put_blocking(pio, sm, reset);
    gpio_put(cs, 1);

    sleep_ms(1);

    // burst length 1024 bytes
    gpio_put(cs, 0);
    uint8_t burst_cmd[] = {APS6404_CMD_BURST_LENGTH, 0x02}; // 1024 bytes
    pio_sm_put_blocking(pio, sm, burst_cmd[0]);
    pio_sm_put_blocking(pio, sm, burst_cmd[1]);
    gpio_put(cs, 1);

    // quad mode
    gpio_put(cs, 0);
    uint8_t quad_enable = APS6404_CMD_ENTER_QUAD;
    pio_sm_put_blocking(pio, sm, quad_enable);
    gpio_put(cs, 1);

    psram->quad_mode = true;

    psram->dma_chan = dma_claim_unused_channel(true);
    
    return true;
}

void aps6404_deinit(APS6404State* psram) 
{
    if (psram->quad_mode) {
        // exit quad mode
        gpio_put(psram->cs_pin, 0);
        uint8_t quad_disable = APS6404_CMD_EXIT_QUAD;
        pio_sm_put_blocking(psram->pio, psram->sm, quad_disable);
        gpio_put(psram->cs_pin, 1);
    }
    
    pio_sm_set_enabled(psram->pio, psram->sm, false);
    dma_channel_unclaim(psram->dma_chan);
}

bool aps6404_write(APS6404State* psram, uint32_t addr, const uint8_t* data, size_t len) 
{
    if (!len) 
        return true;
    
    gpio_put(psram->cs_pin, 0);

    uint32_t cmd_addr = (psram->quad_mode ? APS6404_CMD_WRITE_QUAD : APS6404_CMD_WRITE) | ((addr & 0xFFFFFF) << 8);
    
    pio_sm_put_blocking(psram->pio, psram->sm, cmd_addr);
    
    dma_channel_config config = dma_channel_get_default_config(psram->dma_chan);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, pio_get_dreq(psram->pio, psram->sm, true));

    dma_channel_configure(psram->dma_chan, &config, &psram->pio->txf[psram->sm], data, len,true);

    dma_channel_wait_for_finish_blocking(psram->dma_chan);
    
    gpio_put(psram->cs_pin, 1);

    return true;
}

bool aps6404_read(APS6404State* psram, uint32_t addr, uint8_t* data, size_t len) 
{
    if (!len) 
        return true;
    
    gpio_put(psram->cs_pin, 0);

    uint32_t cmd_addr = (psram->quad_mode ? APS6404_CMD_FAST_READ_QUAD : APS6404_CMD_READ) |((addr & 0xFFFFFF) << 8);
    
    pio_sm_put_blocking(psram->pio, psram->sm, cmd_addr);
    
    if (psram->quad_mode) {
        // dummy cycles for qspi
        pio_sm_put_blocking(psram->pio, psram->sm, 0xFF);
    }
    
    dma_channel_config config = dma_channel_get_default_config(psram->dma_chan);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_read_increment(&config, false);
    channel_config_set_write_increment(&config, true);
    channel_config_set_dreq(&config, pio_get_dreq(psram->pio, psram->sm, false));

    dma_channel_configure(psram->dma_chan, &config, data, &psram->pio->rxf[psram->sm], len, true);

    dma_channel_wait_for_finish_blocking(psram->dma_chan);
    
    gpio_put(psram->cs_pin, 1);

    return true;
}

MemTestResult aps6404_test(APS6404State* psram) 
{
    MemTestResult result = {
        .passed = true,
        .failed_address = 0,
        .expected = 0,
        .received = 0
    };
    
    uint8_t patterns[] = { 0x55, 0xAA, 0x33, 0xCC, 0xF0, 0x0F, 0xFF, 0x00 };
    
    uint32_t test_addresses[] = {
        PSRAM_FRAME_BUFFER_BASE,
        PSRAM_SPRITE_BASE,
        PSRAM_TILEMAP_BASE,
        0x300000  // reserved space
    };
    
    uint8_t read_buffer[sizeof(patterns)];
    
    for (size_t i = 0; i < sizeof(test_addresses)/sizeof(test_addresses[0]); i++) 
    {
        uint32_t addr = test_addresses[i];
        
        // write test pattern
        if (!aps6404_write(psram, addr, patterns, sizeof(patterns))) {
            result.passed = false;
            result.failed_address = addr;
            return result;
        }
        
        // read back
        if (!aps6404_read(psram, addr, read_buffer, sizeof(patterns))) {
            result.passed = false;
            result.failed_address = addr;
            return result;
        }
        
        // verify
        for (size_t j = 0; j < sizeof(patterns); j++) {
            if (read_buffer[j] != patterns[j]) {
                result.passed = false;
                result.failed_address = addr + j;
                result.expected = patterns[j];
                result.received = read_buffer[j];
                return result;
            }
        }

        sleep_ms(1);
    }
    
    return result;
}