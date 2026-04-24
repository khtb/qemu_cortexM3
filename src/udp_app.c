#include "udp_app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lwip/api.h"
#include "uart.h"
#include "eth_log.h"
#include <string.h>
#include <stdio.h>

#define UDP_APP_PORT 5000

static void udp_app_thread(void *arg)
{
    struct netconn *conn;
    struct netbuf *buf;
    err_t err;

    LWIP_UNUSED_ARG(arg);

    /* Create a new UDP connection handle */
    conn = netconn_new(NETCONN_UDP);
    if (conn == NULL) {
        uart_printf("[UDP App] Failed to create netconn\n");
        vTaskDelete(NULL);
    }

    /* Bind to port 5000 with default IP address */
    err = netconn_bind(conn, IP_ADDR_ANY, UDP_APP_PORT);
    if (err != ERR_OK) {
        uart_printf("[UDP App] Failed to bind netconn: %d\n", err);
        netconn_delete(conn);
        vTaskDelete(NULL);
    }

    uart_printf("[UDP App] Listening on port %d\n", UDP_APP_PORT);

    while (1) {
        /* Wait for data */
        err = netconn_recv(conn, &buf);

        if (err == ERR_OK) {
            void *data;
            u16_t len;
            ip_addr_t *addr;
            u16_t port;

            netbuf_data(buf, &data, &len);
            addr = netbuf_fromaddr(buf);
            port = netbuf_fromport(buf);

            char msg[128];
            u16_t copy_len = len < (sizeof(msg) - 1) ? len : (sizeof(msg) - 1);
            memcpy(msg, data, copy_len);
            msg[copy_len] = '\0';

            /* Replace newlines for cleaner UART output */
            for (u16_t i = 0; i < copy_len; i++) {
                if (msg[i] == '\n' || msg[i] == '\r') {
                    msg[i] = ' ';
                }
            }

            uart_printf("[UDP App] Rx %d bytes from %s:%d: %s\n", len, ipaddr_ntoa(addr), port, msg);
            eth_printf("[UDP App] Rx: %s\n", msg);

            /* Echo back the packet to the sender */
            netconn_send(conn, buf);

            netbuf_delete(buf);
        }
    }
}

void udp_app_init(void)
{
    xTaskCreate(udp_app_thread, "UDPApp", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
}
