#include "lwip/def.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"
#include "lwip/mem.h"
#include "lwip/opt.h"
#include "lwip/pbuf.h"
#include "lwip/snmp.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "stellaris_eth.h"
#include <string.h>

/* Define those to appropriate values and packet size */
#define IFNAME0 's'
#define IFNAME1 't'

struct ethernetif
{
    struct eth_addr *ethaddr;
    /* Add objects per-interface here */
};

static void low_level_init(struct netif *netif)
{
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set MAC hardware address */
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x11;
    netif->hwaddr[2] = 0x22;
    netif->hwaddr[3] = 0x33;
    netif->hwaddr[4] = 0x44;
    netif->hwaddr[5] = 0x55;

    /* maximum transfer unit */
    netif->mtu = 1500;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    /* Do hardware initialization. */
    eth_init();
}

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    static uint8_t buffer[1514];
    uint32_t len = 0;

    for (q = p; q != NULL; q = q->next)
    {
        memcpy(&buffer[len], q->payload, q->len);
        len += q->len;
    }

    eth_send(buffer, len);
    uart_puts("[lwIP] Packet Sent\n");

    return ERR_OK;
}

static struct pbuf *low_level_input(struct netif *netif)
{
    struct pbuf *p, *q;
    uint16_t len;
    static uint8_t buffer[1514];

    len = eth_receive(buffer, sizeof(buffer));
    if (len == 0)
        return NULL;

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

    if (p != NULL)
    {
        uint32_t offset = 0;
        for (q = p; q != NULL; q = q->next)
        {
            memcpy(q->payload, &buffer[offset], q->len);
            offset += q->len;
        }
    }

    return p;
}

void ethernetif_input(struct netif *netif)
{
    struct pbuf *p;

    int counts = 0;
    /* move received packets into new pbufs */
    while ((p = low_level_input(netif)) != NULL)
    {
        /* limit processing to prevent starvation */
        if (++counts > 5)
        {
            pbuf_free(p);
            break;
        }

        /* full packet send to tcpip_thread to process */
        if (netif->input(p, netif) != ERR_OK)
        {
            LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
            pbuf_free(p);
            p = NULL;
        }
        else
            uart_puts("[lwIP] Packet Received\n");
    }
}

err_t ethernetif_init(struct netif *netif)
{
    struct ethernetif *ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    ethernetif = mem_malloc(sizeof(struct ethernetif));
    if (ethernetif == NULL)
    {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
        return ERR_MEM;
    }

    netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

    /* initialize the hardware */
    low_level_init(netif);

    return ERR_OK;
}
