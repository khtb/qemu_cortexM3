#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include <stdint.h>

typedef SemaphoreHandle_t sys_sem_t;
typedef SemaphoreHandle_t sys_mutex_t;
typedef QueueHandle_t sys_mbox_t;
typedef TaskHandle_t sys_thread_t;
typedef uint32_t sys_prot_t;

#define sys_sem_valid(sem) (*(sem) != NULL)
#define sys_sem_set_invalid(sem) (*(sem) = NULL)

#define sys_mutex_valid(mutex) (*(mutex) != NULL)
#define sys_mutex_set_invalid(mutex) (*(mutex) = NULL)

#define sys_mbox_valid(mbox) (*(mbox) != NULL)
#define sys_mbox_set_invalid(mbox) (*(mbox) = NULL)

#endif /* LWIP_ARCH_SYS_ARCH_H */
