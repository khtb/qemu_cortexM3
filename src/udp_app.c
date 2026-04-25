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
            ip_addr_t *addr;
            u16_t port;
            u16_t tot_len;

            addr = netbuf_fromaddr(buf);
            port = netbuf_fromport(buf);
            tot_len = netbuf_len(buf);

            if (tot_len == sizeof(comm_packet_t)) {
                comm_packet_t packet;

                /* Safely copy the data from the pbuf chain into our contiguous struct */
                netbuf_copy(buf, &packet, sizeof(comm_packet_t));

                uart_printf("[UDP App] Rx %d bytes from %s:%d (hdr: 0x%08lX, msg_id: %lu, cmd: %lu)\n", 
                            tot_len, ipaddr_ntoa(addr), port, packet.header, packet.msg_id, packet.cmd);
                eth_printf("[UDP App] Comm Req - MsgID: %lu, Cmd: %lu\n", packet.msg_id, packet.cmd);

                /* Process the packet (Dummy Response) */
                packet.cmd = packet.cmd | 0x80000000; // Set MSB to indicate response
                packet.msg_id++; 
                
                /* Copy the modified struct back into the received pbuf chain */
                pbuf_take(buf->p, &packet, sizeof(comm_packet_t));

                /* Echo back the packet to the sender */
                netconn_send(conn, buf);
            } else {
                uart_printf("[UDP App] Rx %d bytes from %s:%d (Ignored, expected %d)\n", 
                            tot_len, ipaddr_ntoa(addr), port, sizeof(comm_packet_t));
            }

            netbuf_delete(buf);
        }
    }
}

void udp_app_init(void)
{
    xTaskCreate(udp_app_thread, "UDPApp", 1024, NULL, tskIDLE_PRIORITY + 2, NULL);
}
