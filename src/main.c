#include "FreeRTOS.h"
#include "ethernetif.h"
#include "logger_task.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "shell.h"
#include "stellaris_eth.h"
#include "task.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>
#include "eth_log.h"
#include "udp_app.h"



// External declarations for shell command registration functions
extern void echo_init(void);
extern void telnet_init(void);

/* Main application task that handles both Shell and Network */
struct netif gnetif;
TaskHandle_t xShellTaskHandle = NULL;

/* Network Task: Polls the Ethernet Controller */
void vTaskNet(void *pvParameters)
{
    (void)pvParameters;
    uart_puts("[NetTask] Started.\n");
    logger_log("NetTask Started");

    TickType_t last_log = 0;

    for (;;)
    {
        ethernetif_input(&gnetif);

        // Log every second
        if (xTaskGetTickCount() - last_log > pdMS_TO_TICKS(1000))
        {
            char buf[32];
            // Minimal snprintf or just manual string logic if no libc
            // Assuming we have basic libc (newlib) from arm-none-eabi
            // sprintf(buf, "Tick: %lu", xTaskGetTickCount());
            // Safety: use a static string for now to avoid libc bloat risks if not linked
            logger_log("System Tick...");
            eth_printf("NetTask Polling Ethernet Interface.\n");
            last_log = xTaskGetTickCount();
        }
        /*
         * Lower delay or no delay might be better for throughput,
         * but we yield to prevent starvation if priorities are equal.
         */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* Shell Task: Handles UART Input/Output */
void vTaskShell(void *pvParameters)
{
    (void)pvParameters;
    uart_puts("[ShellTask] Started.\n");
    eth_printf("ShellTask Started");

    for (;;)
    {
        shell_main();
        /* shell_main blocks on queue/UART usually, but if not we yield */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0); // Disable buffering
    uart_init();
    uart_puts("\n\nHello, QEMU Cortex-M3 with lwIP stack!\n");

    shell_init();

    /* Initialize lwIP TCPIP stack */
    tcpip_init(NULL, NULL);

    ip4_addr_t ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 10, 0, 2, 15);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 10, 0, 2, 2);

    netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
    netif_set_default(&gnetif);
    netif_set_up(&gnetif);

    echo_init();
    telnet_init();
    logger_init();
    udp_app_init();

    /* Create Tasks */
    /* NetTask priority should be higher or equal to tcpip_thread (default 3) ideally,
       but here we keep it simple. Let's make it 3 to match TCPIP_THREAD_PRIO. */
    xTaskCreate(vTaskNet, "NetTask", 1024, NULL, tskIDLE_PRIORITY + 3, NULL);

    /* ShellTask can be lower priority */
    xTaskCreate(vTaskShell, "ShellTask", 1024, NULL, tskIDLE_PRIORITY + 1, &xShellTaskHandle);

    // Start scheduler
    vTaskStartScheduler();

    return 0;
}
