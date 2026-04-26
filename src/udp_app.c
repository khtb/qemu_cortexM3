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

                uart_printf("[UDP App] Rx %d bytes from %s:%d (hdr: 0x%08lX, msg_id: %lu, cmd: 0x%04lX)\n", 
                            tot_len, ipaddr_ntoa(addr), port, packet.header, packet.msg_id, packet.cmd);
                eth_printf("[UDP App] Comm Req - MsgID: %lu, Cmd: 0x%04lX\n", packet.msg_id, packet.cmd);

                if (packet.cmd == CMD_GET_3_FRAMES) {
                    for (int i = 0; i < 3; i++) {
                        packet.cmd = CMD_GET_3_FRAMES | 0x80000000; // Set MSB to indicate response
                        packet.msg_id = i + 1; // Sequence 1, 2, 3
                        
                        // Format a payload message
                        char msg[64];
                        snprintf(msg, sizeof(msg), "This is Frame %d of 3", i + 1);
                        packet.payload_len = strlen(msg);
                        memset(packet.payload, 0, sizeof(packet.payload));
                        memcpy(packet.payload, msg, packet.payload_len);
                        
                        packet.eof_marker = EOF_MARKER_VAL;
                        
                        struct netbuf *resp_buf = netbuf_new();
                        if (resp_buf) {
                            void *data = netbuf_alloc(resp_buf, sizeof(comm_packet_t));
                            if (data) {
                                memcpy(data, &packet, sizeof(comm_packet_t));
                                netconn_sendto(conn, resp_buf, addr, port);
                            }
                            netbuf_delete(resp_buf);
                        }
                        
                        vTaskDelay(pdMS_TO_TICKS(10)); // Slight delay between frames
                    }
                } else {
                    /* Default Echo Process */
                    packet.cmd = packet.cmd | 0x80000000; // Set MSB to indicate response
                    packet.msg_id++; 
                    packet.eof_marker = EOF_MARKER_VAL;
                    
                    struct netbuf *resp_buf = netbuf_new();
                    if (resp_buf) {
                        void *data = netbuf_alloc(resp_buf, sizeof(comm_packet_t));
                        if (data) {
                            memcpy(data, &packet, sizeof(comm_packet_t));
                            netconn_sendto(conn, resp_buf, addr, port);
                        }
                        netbuf_delete(resp_buf);
                    }
                }
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
