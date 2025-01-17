#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"

#include "gpu/aps6404.h"
#include "gpu/display.h"
#include "gpu/sprite_engine.h"
#include "gpu/command_queue.h"
#include "gpu/gpu_protocol.h"
#include "gpu/gpu_status.h"
#include "externs.h"
#include "pins.h"

static CommandQueue cmd_queue;
static TransferState transfer_state;
static volatile bool system_initialized = false;

static void init_led(void);
static bool init_hardware(void);
static void process_command(const uint8_t* cmd_data, size_t cmd_len);

int main() {
    stdio_init_all();
    printf("\nRetro GPU Starting...\n");

    init_led();
    gpio_put(PIN_LED, 1);  // Turn on LED during init

    if (!init_hardware()) {
        printf("Hardware initialization failed!\n");
        while(1) {
            gpio_put(PIN_LED, 1);
            sleep_ms(100);
            gpio_put(PIN_LED, 0);
            sleep_ms(100);
        }
    }

    //printf("Running display test...\n");
    //run_display_test();

    //sleep_ms(2000);

    ///printf("Running sprite test...\n");
    //run_sprite_test();

    printf("Entering main loop\n");
    system_initialized = true;
    
    uint32_t frame_count = 0;
    uint32_t last_time = time_us_32();
    
    while (1) {
        frame_count++;
        
        // process pending commands
        static uint8_t cmd_buffer[256];
        uint16_t cmd_len;
        bool needs_response;
        uint16_t response_len;
        
        while (cmd_queue_has_command(&cmd_queue)) {
            if (cmd_queue_pop(&cmd_queue, cmd_buffer, &cmd_len,
                            &needs_response, &response_len)) {
                process_command(cmd_buffer, cmd_len);
            }
        }

        sprite_engine_start_frame();

        display_wait_for_frame_complete();

        if ((frame_count % 60) == 0) {
            gpio_put(PIN_LED, !gpio_get(PIN_LED));

            uint32_t current_time = time_us_32();
            float fps = 60.0f / ((current_time - last_time) / 1000000.0f);
            printf("FPS: %.2f\n", fps);
            last_time = current_time;
        }
    }

    return 0;
}

static void init_led(void) {
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 0);
}

static bool init_hardware(void) {
    printf("Initializing hardware...\n");

    // initialize PSRAM
    printf("Initializing PSRAM...\n");
    if (!aps6404_init(&psram, pio0, 0,
                      PIN_PSRAM_SCK,
                      PIN_PSRAM_D0,
                      PIN_PSRAM_D1,
                      PIN_PSRAM_D2,
                      PIN_PSRAM_D3,
                      PIN_PSRAM_CS)) {
        printf("PSRAM initialization failed!\n");
        return false;
    }

    // test PSRAM
    printf("Testing PSRAM...\n");
    MemTestResult test_result = aps6404_test(&psram);
    if (!test_result.passed) {
        printf("PSRAM test failed at address 0x%08x\n", test_result.failed_address);
        printf("Expected: 0x%02x, Received: 0x%02x\n", 
               test_result.expected, test_result.received);
        return false;
    }

    // init display
    printf("Initializing display...\n");
    if (!display_init(pio1, 0)) {
        printf("Display initialization failed!\n");
        return false;
    }

    // init sprite engine
    printf("Initializing sprite engine...\n");
    if (!sprite_engine_init(pio2, 0, 1, 2)) {
        printf("Sprite engine initialization failed!\n");
        return false;
    }

    // init command queue
    printf("Initializing command queue...\n");
    cmd_queue_init(&cmd_queue);

    // init transfer system
    printf("Initializing transfer system...\n");
    if (!transfer_init(&transfer_state, pio0, 1)) {
        printf("Transfer system initialization failed!\n");
        return false;
    }

    printf("Hardware initialization complete!\n");
    return true;
}

static void process_command(const uint8_t* cmd_data, size_t cmd_len) {
    if (cmd_len < sizeof(GpuCommandHeader)) return;

    GpuCommandHeader* header = (GpuCommandHeader*)cmd_data;
    const uint8_t* data = cmd_data + sizeof(GpuCommandHeader);
    
    // Reset sprite engine if needed
    if (cmd_resets_state(header)) {
        //todo
        //sprite_engine_reset();

    }
    
    // handle high priority commands first
    if (cmd_is_high_priority(header)) {
        // lol eventually
    }

    switch(header->cmd) 
    {
        case CMD_INIT:
            if (cmd_needs_response(header)) 
            {
                uint8_t ack = 0;
                transfer_send_response(&transfer_state, &ack, sizeof(ack));
            }
            break;
            
        case CMD_LOAD_PATTERN: 
        {
            if (cmd_len < sizeof(GpuCommandHeader) + sizeof(LoadPatternData)) break;
            LoadPatternData* pattern = (LoadPatternData*)data;
            size_t pattern_data_size;

            switch(pattern->size) 
            {
                case SPRITE_SIZE_8x8:    
                    pattern_data_size = 32;   
                    break;
                case SPRITE_SIZE_16x16:  
                    pattern_data_size = 128;  
                    break;
                case SPRITE_SIZE_32x32:  
                    pattern_data_size = 512;  
                    break;
                case SPRITE_SIZE_64x64:  
                    pattern_data_size = 2048; 
                    break;
                default: 
                    if (cmd_needs_response(header)) 
                    {
                        bool success = false;
                        transfer_send_response(&transfer_state, &success, sizeof(success));
                    }
                    break;
            }

            if (cmd_len < sizeof(GpuCommandHeader) + sizeof(LoadPatternData) + pattern_data_size)
            {   
                break;
            } 

            bool success = pattern_load(pattern->pattern_num, data + sizeof(LoadPatternData), pattern->size);
            
            if (cmd_needs_response(header)) 
            {
                transfer_send_response(&transfer_state, &success, sizeof(success));
            }

            break;
        }
        
        case CMD_UPDATE_SPRITE: 
        {
            if (cmd_len < sizeof(GpuCommandHeader) + sizeof(UpdateSpriteData)) break;
            UpdateSpriteData* spriteUpdateData = (UpdateSpriteData*)data;
            
            // Check if sprite number is within bounds
            if (spriteUpdateData->sprite_num >= MAX_SPRITES) 
            {
                bool success = false;
                if (cmd_needs_response(header)) 
                {
                    transfer_send_response(&transfer_state, &success, sizeof(success));
                }
                
                break;
            }
            
            Sprite sprite;
            sprite.x = spriteUpdateData->x;
            sprite.y = spriteUpdateData->y;
            sprite.pattern = spriteUpdateData->pattern;
            sprite.attr = spriteUpdateData->attr;
            sprite.ctrl = spriteUpdateData->ctrl;
            
            bool success = sprite_update((uint8_t)spriteUpdateData->sprite_num, &sprite);
            
            if (cmd_needs_response(header)) 
            {
                transfer_send_response(&transfer_state, &success, sizeof(success));
            }

            break;
        }
        
        case CMD_LOAD_PALETTE: 
        {
            if (cmd_len < sizeof(GpuCommandHeader) + sizeof(LoadPaletteData))
            { 
                break;
            }

            LoadPaletteData* palette = (LoadPaletteData*)data;

            bool success = palette_load(palette->palette_num, (uint16_t*)(data + sizeof(LoadPaletteData)));
            
            if (cmd_needs_response(header)) 
            {
                transfer_send_response(&transfer_state, &success, sizeof(success));
            }

            break;
        }
        
        case CMD_STATUS: 
        {
            GpuStatus status = gpu_get_status();
            
            if (cmd_needs_response(header)) 
            {
                transfer_send_response(&transfer_state, &status, sizeof(status));
            }
            
            break;
        }

        default:
            if (cmd_needs_response(header)) 
            {
                uint8_t error = 1;
                transfer_send_response(&transfer_state, &error, sizeof(error));
            }
            break;
    }
}