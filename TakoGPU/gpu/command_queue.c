#include "command_queue.h"
#include "hardware/sync.h"
#include <string.h>

void cmd_queue_init(CommandQueue* queue) 
{
    queue->read_idx = 0;
    queue->write_idx = 0;
    queue->buffer_write_pos = 0;
    queue->processing = false;
    queue->error_code = 0;

    queue->lock = spin_lock_init(spin_lock_claim_unused(true));
}

static inline uint8_t queue_next_idx(uint8_t idx) 
{
    return (idx + 1) & (CMD_QUEUE_SIZE - 1);
}

static inline uint16_t buffer_wrap_pos(uint16_t pos) 
{
    return pos >= CMD_DATA_BUFFER_SIZE ? 0 : pos;
}

bool cmd_queue_push(CommandQueue* queue, const void* cmd_data, uint16_t cmd_len, bool needs_response, uint16_t response_len) 
{
    if (!cmd_data || !cmd_len || cmd_len > CMD_DATA_BUFFER_SIZE) 
    {
        return false;
    }
    
    uint32_t save = spin_lock_blocking(queue->lock);
    
    // is queue is full
    if (queue_next_idx(queue->write_idx) == queue->read_idx) {
        spin_unlock(queue->lock, save);
        return false;
    }
    
    // enough space in data buffer?
    uint16_t needed_space = cmd_len;
    if (queue->buffer_write_pos + needed_space > CMD_DATA_BUFFER_SIZE) 
    {
        // wrap
        if (queue->read_idx == queue->write_idx) 
        {
            // no commands in queue, can reset buffer
            queue->buffer_write_pos = 0;
        } 
        else 
        {
            // queue is effectively full
            spin_unlock(queue->lock, save);
            return false;
        }
    }
    
    // copy command data to buffer
    memcpy(&queue->data_buffer[queue->buffer_write_pos], cmd_data, cmd_len);
    
    QueuedCommand* cmd = &queue->commands[queue->write_idx];
    memcpy(&cmd->header, cmd_data, sizeof(GpuCommandHeader));
    cmd->data_offset = queue->buffer_write_pos;
    cmd->data_length = cmd_len;
    cmd->needs_response = needs_response;
    cmd->response_length = response_len;
    
    queue->buffer_write_pos += cmd_len;
    queue->write_idx = queue_next_idx(queue->write_idx);
    
    spin_unlock(queue->lock, save);
    return true;
}

bool cmd_queue_pop(CommandQueue* queue, void* cmd_buffer, uint16_t* cmd_len, bool* needs_response, uint16_t* response_len) 
{
    uint32_t save = spin_lock_blocking(queue->lock);
    
    // queue is empty?
    if (queue->read_idx == queue->write_idx) 
    {
        spin_unlock(queue->lock, save);
        return false;
    }
    
    QueuedCommand* cmd = &queue->commands[queue->read_idx];

    memcpy(cmd_buffer, &queue->data_buffer[cmd->data_offset], cmd->data_length);
    *cmd_len = cmd->data_length;
    *needs_response = cmd->needs_response;
    *response_len = cmd->response_length;
    
    // update read index
    queue->read_idx = queue_next_idx(queue->read_idx);
    
    spin_unlock(queue->lock, save);
    return true;
}

bool cmd_queue_is_full(CommandQueue* queue) 
{
    uint32_t save = spin_lock_blocking(queue->lock);
    bool full = queue_next_idx(queue->write_idx) == queue->read_idx;
    spin_unlock(queue->lock, save);
    return full;
}

bool cmd_queue_is_empty(CommandQueue* queue) 
{
    uint32_t save = spin_lock_blocking(queue->lock);
    bool empty = queue->read_idx == queue->write_idx;
    spin_unlock(queue->lock, save);
    return empty;
}

bool cmd_queue_has_command(CommandQueue* queue) 
{
    return !cmd_queue_is_empty(queue);
}

uint8_t cmd_queue_get_count(CommandQueue* queue) 
{
    uint32_t save = spin_lock_blocking(queue->lock);
    uint8_t count = (queue->write_idx - queue->read_idx) & (CMD_QUEUE_SIZE - 1);
    spin_unlock(queue->lock, save);
    return count;
}

uint8_t cmd_queue_get_error(CommandQueue* queue) 
{
    return queue->error_code;
}

void cmd_queue_clear_error(CommandQueue* queue) 
{
    queue->error_code = 0;
}