#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

extern struct netif gnetif;

static BaseType_t prvNetStatusCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                      const char *pcCommandString)
{
    (void)pcCommandString;

    char ip[16], nm[16], gw[16];

    strcpy(ip, ipaddr_ntoa(netif_ip4_addr(&gnetif)));
    strcpy(nm, ipaddr_ntoa(netif_ip4_netmask(&gnetif)));
    strcpy(gw, ipaddr_ntoa(netif_ip4_gw(&gnetif)));

    snprintf(pcWriteBuffer, xWriteBufferLen,
             "Network Interface: %c%c%d\r\n"
             "IP Address: %s\r\n"
             "Netmask: %s\r\n"
             "Gateway: %s\r\n"
             "Status: %s\r\n",
             gnetif.name[0], gnetif.name[1], gnetif.num, ip, nm, gw,
             netif_is_up(&gnetif) ? "Up" : "Down");

    return pdFALSE;
}

static const CLI_Command_Definition_t xNetStatus = {
    "net-status", "net-status: Shows network interface information\r\n", prvNetStatusCommand, 0};

void vRegisterNetCommands(void) { FreeRTOS_CLIRegisterCommand(&xNetStatus); }
