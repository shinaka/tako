#include "gpu_status.h"
#include "hardware/irq.h"
#include "hardware/sync.h"

static volatile GpuStatus current_status = {
    .status = GPU_STATUS_OK,
    .busy = 0,
    .error_code = GPU_ERROR_NONE,
    .sprite_count = 0,
    .frame_rate = 60,
    .reserved = {0, 0, 0}
};

// todo - pretty sure this interrupt handling isn't going to work
GpuStatus gpu_get_status(void) 
{
    GpuStatus status;
    uint32_t iStatus = save_and_disable_interrupts();
    status = current_status;
    
    restore_interrupts(iStatus);
    
    return status;
}

void gpu_clear_error(void) 
{
    uint32_t iStatus = save_and_disable_interrupts();
    current_status.status = GPU_STATUS_OK;
    current_status.error_code = GPU_ERROR_NONE;
    
    restore_interrupts(iStatus);
}

void gpu_set_busy_flag(uint8_t flag) 
{
    uint32_t iStatus = save_and_disable_interrupts();
    current_status.busy |= flag;
    
    if (current_status.busy) 
    {
        current_status.status = GPU_STATUS_BUSY;
    }
    
    restore_interrupts(iStatus);
}

void gpu_clear_busy_flag(uint8_t flag) 
{
    uint32_t iStatus = save_and_disable_interrupts();
    current_status.busy &= ~flag;
    
    if (!current_status.busy) 
    {
        current_status.status = GPU_STATUS_OK;
    }
    
    restore_interrupts(iStatus);
}

void gpu_set_error(GpuErrorCode error) 
{
    uint32_t iStatus = save_and_disable_interrupts();
    current_status.status = GPU_STATUS_ERROR;
    current_status.error_code = error;
    
    restore_interrupts(iStatus);
}