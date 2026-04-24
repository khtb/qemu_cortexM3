#include "FreeRTOS.h"
#include "lwip/opt.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "task.h"
#include "FreeRTOS_CLI.h"
#include "uart.h"
#include "eth_log.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>

#define PING_ID 0xAFAF
#define PING_DATA_SIZE 32
#define PING_DELAY_MS 1000

static u32_t ping_time;
static volatile u32_t ping_received_seq;
static volatile u32_t ping_received_count;
static int ping_run_step = 0;
static struct raw_pcb *raw_ping_pcb = NULL;
static ip_addr_t target_addr;
static int count = 0;

#include <stdarg.h>

static void shell_async_printf(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    uart_puts(buf);
    eth_printf("[PING] %s", buf);
    telnet_puts(buf);
}

static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    struct icmp_echo_hdr *iecho;
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(addr);

    if (p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr)))
    {
        iecho = (struct icmp_echo_hdr *)((u8_t *)p->payload + PBUF_IP_HLEN); // Skip IP header

        uart_printf("RX ICMP: type=%d id=0x%04X seq=0x%04X (expected id=0x%04X seq=0x%04X)\r\n", 
                    iecho->type, iecho->id, iecho->seqno, PING_ID, htons(ping_received_seq));

        if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_received_seq)))
        {
            /* Checksum is checked by lwIP raw layer if configured, or we assume it's good */
            if (iecho->type == ICMP_ER)
            {
                uart_printf("Reply from %s: bytes=%d time=%dms TTL=%d\n", ipaddr_ntoa(addr),
                            p->tot_len - PBUF_IP_HLEN - sizeof(struct icmp_echo_hdr),
                            xTaskGetTickCount() - ping_time, ((struct ip_hdr *)p->payload)->_ttl);
                eth_printf("[PING] Reply from %s, time=%dms\n", ipaddr_ntoa(addr), xTaskGetTickCount() - ping_time);
                ping_received_count++;
            }
        }
    }

    pbuf_free(p);
    return 1; /* Eat the packet */
}

static void ping_send(struct raw_pcb *raw, const ip_addr_t *addr)
{
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p)
    {
        return;
    }

    if ((p->len == p->tot_len) && (p->next == NULL))
    {
        iecho = (struct icmp_echo_hdr *)p->payload;

        iecho->type = ICMP_ECHO;
        iecho->code = 0;
        iecho->chksum = 0;
        iecho->id = PING_ID;
        iecho->seqno = htons(++ping_received_seq);

        /* Fill the additional data buffer with some data */
        for (size_t i = 0; i < PING_DATA_SIZE; i++)
        {
            ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
        }

        iecho->chksum = inet_chksum(iecho, ping_size);

        ping_time = xTaskGetTickCount();
        raw_sendto(raw, p, addr);
    }

    pbuf_free(p);
}

static void ping_timer_callback(void *arg);

static void ping_do_send(void *arg)
{
    if (!raw_ping_pcb) return;

    if (ping_run_step <= count)
    {
        ping_send(raw_ping_pcb, &target_addr);
        ping_run_step++;
        sys_timeout(1000, ping_timer_callback, NULL);
    }
    else
    {
        shell_async_printf("\r\nPing statistics for %s:\r\n    Packets: Sent = %d, Received = %d, Lost = %d (%.0f%% loss)\r\n> ",
                 ipaddr_ntoa(&target_addr), count, (int)ping_received_count,
                 count - (int)ping_received_count,
                 (double)(count - ping_received_count) / count * 100);
        
        raw_remove(raw_ping_pcb);
        raw_ping_pcb = NULL;
        ping_run_step = 0;
    }
}

static void ping_timer_callback(void *arg)
{
    tcpip_callback(ping_do_send, NULL);
}

static void ping_do_init(void *arg)
{
    if (raw_ping_pcb) return; // Already running

    raw_ping_pcb = raw_new(IP_PROTO_ICMP);
    if (!raw_ping_pcb) return;

    raw_recv(raw_ping_pcb, ping_recv, NULL);
    raw_bind(raw_ping_pcb, IP_ADDR_ANY);

    ping_run_step = 1;
    ping_do_send(NULL);
}

BaseType_t prvPingCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    if (ping_run_step != 0)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Ping is already running\n");
        return pdFALSE;
    }

    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameter == NULL)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Usage: ping <ip_address>\n");
        return pdFALSE;
    }

    char ip_str[16];
    if (xParameterStringLength > 15) xParameterStringLength = 15;
    strncpy(ip_str, pcParameter, xParameterStringLength);
    ip_str[xParameterStringLength] = '\0';

    if (!ipaddr_aton(ip_str, &target_addr))
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Invalid IP address\n");
        return pdFALSE;
    }

    count = 4;
    ping_received_seq = 0;
    ping_received_count = 0;
    snprintf(pcWriteBuffer, xWriteBufferLen, "Pinging %s with 32 bytes of data:\n", ip_str);

    tcpip_callback(ping_do_init, NULL);

    return pdFALSE;
}

static const CLI_Command_Definition_t xPingCommand = {
    "ping", "\r\nping <ip>:\r\n Ping an IP address\r\n", prvPingCommand, 1};
