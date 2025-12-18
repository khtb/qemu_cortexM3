# LM3S6965 Ethernet Bring-Up Steps (FreeRTOS + lwIP)

## **1. Enable Required Clocks**

-   Enable the clock for:
    -   Ethernet MAC (EMAC)
    -   Ethernet PHY
    -   GPIO ports used for RMII/MII pins
-   Verify PLL or system clock meets EMAC timing requirements.

``` c
SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF); // example for RMII pins
```

## **2. Configure GPIO Pins for EMAC / RMII**

-   Configure the GPIO pins for RMII function.
-   Set alternate functions + pad configuration.

``` c
GPIOPinConfigure(GPIO_PF2_EN0RXD0);
GPIOPinConfigure(GPIO_PF3_EN0RXD1);
GPIOPinConfigure(GPIO_PF4_EN0TXEN);
GPIOPinConfigure(GPIO_PF0_EN0TXD0);
GPIOPinConfigure(GPIO_PF1_EN0TXD1);
```

## **3. Reset and Initialize the EMAC Peripheral**

``` c
EMACReset(ETH_BASE);
while (EMACStatus(ETH_BASE) & EMAC_STATUS_RESET);
```

## **4. Configure PHY (Internal / External)**

-   Auto-negotiation
-   Link speed
-   Duplex
-   Poll until link OK

## **5. Initialize lwIP Memory Structures**

``` c
#define NO_SYS 0
#define MEM_LIBC_MALLOC 0
#define MEM_ALIGNMENT 4
#define PBUF_POOL_SIZE 16
```

## **6. Initialize the EMAC Driver**

``` c
void low_level_init(struct netif *netif)
{
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    EMACInit(ETH_BASE, sys_clock, EMAC_MODE_FULL_DUPLEX);
    EMACAddrSet(ETH_BASE, 0, mac);
}
```

## **7. Setup DMA Descriptors (RX/TX)**

## **8. Register EMAC Interrupts**

``` c
IntEnable(INT_ETH);
EMACIntEnable(ETH_BASE, EMAC_INT_RECEIVE | EMAC_INT_TRANSMIT);
```

## **9. Create FreeRTOS Network Task**

``` c
xTaskCreate(EthernetInputTask, "eth", 512, NULL, 4, NULL);
```

## **10. Assign IP Address**

``` c
IP4_ADDR(&ipaddr, 192,168,1,20);
IP4_ADDR(&netmask,255,255,255,0);
IP4_ADDR(&gateway,192,168,1,1);
```

## **11. Start lwIP Services**

## **12. Verify Communication**
