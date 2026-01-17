#include "FreeRTOS.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/sys.h"
#include "semphr.h"
#include "task.h"

/* Porting layer for FreeRTOS */

void sys_init(void) { /* No special initialization needed for FreeRTOS port */ }

u32_t sys_now(void) { return xTaskGetTickCount() * (1000 / configTICK_RATE_HZ); }

err_t sys_sem_new(sys_sem_t *sem, u8_t count)
{
    *sem = xSemaphoreCreateBinary();
    if (*sem == NULL)
    {
        return ERR_MEM;
    }
    if (count > 0)
    {
        xSemaphoreGive(*sem);
    }
    return ERR_OK;
}

void sys_sem_free(sys_sem_t *sem) { vSemaphoreDelete(*sem); }

void sys_sem_signal(sys_sem_t *sem) { xSemaphoreGive(*sem); }

u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t wait_ticks =
        (timeout == 0) ? portMAX_DELAY : (timeout / (1000 / configTICK_RATE_HZ));

    if (xSemaphoreTake(*sem, wait_ticks) == pdTRUE)
    {
        return (xTaskGetTickCount() - start_tick) * (1000 / configTICK_RATE_HZ);
    }
    return SYS_ARCH_TIMEOUT;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
    if (size <= 0)
    {
        size = 16; /* Default size if not specified */
    }
    *mbox = xQueueCreate(size, sizeof(void *));
    if (*mbox == NULL)
    {
        return ERR_MEM;
    }
    return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox) { vQueueDelete(*mbox); }

void sys_mbox_post(sys_mbox_t *mbox, void *msg) { xQueueSend(*mbox, &msg, portMAX_DELAY); }

err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
    if (xQueueSend(*mbox, &msg, 0) == pdTRUE)
    {
        return ERR_OK;
    }
    return ERR_MEM;
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
    void *temp_msg;
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t wait_ticks =
        (timeout == 0) ? portMAX_DELAY : (timeout / (1000 / configTICK_RATE_HZ));

    if (xQueueReceive(*mbox, &temp_msg, wait_ticks) == pdTRUE)
    {
        if (msg != NULL)
        {
            *msg = temp_msg;
        }
        return (xTaskGetTickCount() - start_tick) * (1000 / configTICK_RATE_HZ);
    }
    return SYS_ARCH_TIMEOUT;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
    void *temp_msg;
    if (xQueueReceive(*mbox, &temp_msg, 0) == pdTRUE)
    {
        if (msg != NULL)
        {
            *msg = temp_msg;
        }
        return 0;
    }
    return SYS_ARCH_TIMEOUT;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (xQueueSendFromISR(*mbox, &msg, &xHigherPriorityTaskWoken) == pdTRUE)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return ERR_OK;
    }
    return ERR_MEM;
}

err_t sys_mutex_new(sys_mutex_t *mutex)
{
    *mutex = xSemaphoreCreateMutex();
    if (*mutex == NULL)
    {
        return ERR_MEM;
    }
    return ERR_OK;
}

void sys_mutex_lock(sys_mutex_t *mutex) { xSemaphoreTake(*mutex, portMAX_DELAY); }

void sys_mutex_unlock(sys_mutex_t *mutex) { xSemaphoreGive(*mutex); }

void sys_mutex_free(sys_mutex_t *mutex) { vSemaphoreDelete(*mutex); }

sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize,
                            int prio)
{
    TaskHandle_t handle;
    xTaskCreate((TaskFunction_t)thread, name, stacksize, arg, prio, &handle);
    return handle;
}

sys_prot_t sys_arch_protect(void)
{
    vPortEnterCritical();
    return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
    (void)pval;
    vPortExitCritical();
}
