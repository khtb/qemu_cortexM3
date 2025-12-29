#include <stdint.h>
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stellaris_eth.h"
#include "shell.h"

void vTaskCounter(void *pvParameters)
{
    /* 100 msec */
    for (;;)
    {
        static uint8_t ctr = 0;
        shell_main();
        eth_poll();
        if (ctr==10)
        {
            ctr = 0;
            shell_log("INFO 100msec");
        }else{
            ctr++;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


int main(void)
{
    uart_init();
    uart_puts("Hello, MPS2 lm3s6965evb (Cortex-M3) via UART0!\n");
    eth_init();
    shell_init();
    xTaskCreate(vTaskCounter, "CounterTask", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
    // Start scheduler
    vTaskStartScheduler();
}
