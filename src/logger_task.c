#include "logger_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "stellaris_eth.h"
#include "task.h"
#include "uart.h"
#include <string.h>

#define LOGGER_QUEUE_SIZE 10
#define LOGGER_MSG_MAX_LEN 128
#define ETH_TYPE_LOG 0x88B5

// Frame buffer type
typedef struct
{
    char msg[LOGGER_MSG_MAX_LEN];
} LogMessage;

static QueueHandle_t xLogQueue = NULL;

void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    LogMessage logMsg;
    uint8_t eth_frame[1500];

    // Ethernet Header Construction
    // Destination: Broadcast
    memset(eth_frame, 0xFF, 6);
    // Source: 00:11:22:33:44:55 (Matches eth_init)
    eth_frame[6] = 0x00;
    eth_frame[7] = 0x11;
    eth_frame[8] = 0x22;
    eth_frame[9] = 0x33;
    eth_frame[10] = 0x44;
    eth_frame[11] = 0x55;
    // EtherType (Big Endian)
    eth_frame[12] = (ETH_TYPE_LOG >> 8) & 0xFF;
    eth_frame[13] = ETH_TYPE_LOG & 0xFF;

    for (;;)
    {
        if (xQueueReceive(xLogQueue, &logMsg, portMAX_DELAY) == pdTRUE)
        {
            int len = strlen(logMsg.msg);
            if (len > 0)
            {
                // Copy payload
                memcpy(&eth_frame[14], logMsg.msg, len);

                // Send frame (Header 14 + Payload)
                eth_send(eth_frame, 14 + len);
            }
        }
    }
}

void logger_init(void)
{
    xLogQueue = xQueueCreate(LOGGER_QUEUE_SIZE, sizeof(LogMessage));

    // Priority should be low, but high enough to drain queue
    xTaskCreate(vLoggerTask, "Logger", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}

void logger_log(const char *msg)
{
    if (xLogQueue != NULL)
    {
        LogMessage l;
        strncpy(l.msg, msg, LOGGER_MSG_MAX_LEN - 1);
        l.msg[LOGGER_MSG_MAX_LEN - 1] = '\0';

        // Don't block if full, just drop
        xQueueSend(xLogQueue, &l, 0);
    }
}
