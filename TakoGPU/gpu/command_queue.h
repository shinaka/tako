#pragma once

#include <stdint.h>
#include "hardware/sync.h"
#include "gpu_protocol.h"

#define CMD_QUEUE_SIZE 32
#define CMD_DATA_BUFFER_SIZE 2048

// Command queue entry
typedef struct {
    GpuCommandHeader header;
    uint16_t data_offset; // offset into data buffer
    uint16_t data_length; // length of command data
    bool needs_response; 
    uint16_t response_length;
} QueuedCommand;

// Command queue structure
typedef struct {
    // ring buffer
    QueuedCommand commands[CMD_QUEUE_SIZE];
    volatile uint8_t read_idx;
    volatile uint8_t write_idx;
    
    // data buffer (circular)
    uint8_t data_buffer[CMD_DATA_BUFFER_SIZE];
    uint16_t buffer_write_pos;
    
    spin_lock_t *lock;
    
    volatile bool processing;
    volatile uint8_t error_code;
} CommandQueue;

void cmd_queue_init(CommandQueue* queue);

bool cmd_queue_push(CommandQueue* queue, const void* cmd_data, uint16_t cmd_len, bool needs_response, uint16_t response_len);
                   
bool cmd_queue_pop(CommandQueue* queue, void* cmd_buffer, uint16_t* cmd_len, bool* needs_response, uint16_t* response_len);

bool cmd_queue_is_full(CommandQueue* queue);
bool cmd_queue_is_empty(CommandQueue* queue);
bool cmd_queue_has_command(CommandQueue* queue);
uint8_t cmd_queue_get_count(CommandQueue* queue);

uint8_t cmd_queue_get_error(CommandQueue* queue);
void cmd_queue_clear_error(CommandQueue* queue);