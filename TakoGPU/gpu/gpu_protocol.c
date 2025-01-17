#include "gpu_protocol.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "gpu_transfer.pio.h"
#include "../pins.h"

bool transfer_init(TransferState* state, PIO pio, uint sm) 
{
    state->pio = pio;
    state->sm = sm;
    state->transfer_active = false;
    state->waiting_for_response = false;

    state->offset = pio_add_program(pio, &gpu_transfer_program);
    
    pio_sm_config c = gpu_transfer_program_get_default_config(state->offset);
    
    sm_config_set_out_pins(&c, PIN_D0, 8); 
    sm_config_set_in_pins(&c, PIN_D0);
    sm_config_set_sideset_pins(&c, PIN_WAIT);
    
    for(int i = 0; i < 8; i++) 
    {
        pio_gpio_init(pio, PIN_D0 + i);
    }
    
    pio_gpio_init(pio, PIN_WAIT);
    gpio_init(PIN_CS);
    gpio_init(PIN_RW);
    
    
    gpio_set_dir(PIN_CS, GPIO_IN); 
    gpio_set_dir(PIN_RW, GPIO_IN);
    gpio_set_dir(PIN_WAIT, GPIO_OUT);
    
    gpio_put(PIN_WAIT, 1);
    
    pio_sm_init(pio, sm, state->offset, &c);
    pio_sm_set_enabled(pio, sm, true);
    
    return true;
}

bool transfer_send_data(TransferState* state, const void* data, size_t len) 
{
    if (state->transfer_active) 
        return false;
    
    state->transfer_active = true;
    const uint8_t* bytes = (const uint8_t*)data;
    
    gpio_set_dir_out_masked(0xFF << PIN_D0); // data pins as outputs
    
    for(size_t i = 0; i < len; i++) 
    {
        pio_sm_set_enabled(state->pio, state->sm, false);
        pio_sm_clear_fifos(state->pio, state->sm);
        pio_sm_restart(state->pio, state->sm);
        pio_sm_exec(state->pio, state->sm, pio_encode_jmp(state->offset + gpu_transfer_offset_send));
        pio_sm_set_enabled(state->pio, state->sm, true);
        
        pio_sm_put_blocking(state->pio, state->sm, bytes[i]);
        
        pio_sm_get_blocking(state->pio, state->sm);
    }
    
    state->transfer_active = false;
    return true;
}

bool transfer_receive_data(TransferState* state, void* data, size_t len) 
{
    if (state->transfer_active) return false;
    
    state->transfer_active = true;
    uint8_t* bytes = (uint8_t*)data;

    gpio_set_dir_in_masked(0xFF << PIN_D0); // data pins as inputs

    for(size_t i = 0; i < len; i++) 
    {
        pio_sm_set_enabled(state->pio, state->sm, false);
        pio_sm_clear_fifos(state->pio, state->sm);
        pio_sm_restart(state->pio, state->sm);
        pio_sm_exec(state->pio, state->sm, pio_encode_jmp(state->offset + gpu_transfer_offset_receive));
        pio_sm_set_enabled(state->pio, state->sm, true);
        
        bytes[i] = pio_sm_get_blocking(state->pio, state->sm);
    }
    
    state->transfer_active = false;

    return true;
}

bool transfer_send_response(TransferState* state, const void* data, size_t len) 
{
    gpio_put(PIN_WAIT, 0);
    bool result = transfer_send_data(state, data, len);
    gpio_put(PIN_WAIT, 1);
    return result;
}

bool transfer_wait_response(TransferState* state, void* data, size_t len) 
{
    state->waiting_for_response = true;
    bool result = transfer_receive_data(state, data, len);
    state->waiting_for_response = false;
    return result;
}