#ifndef STELLARIS_ETH_H
#define STELLARIS_ETH_H

#include <stdint.h>

#define ETH_BASE 0x40048000
#define MAC_RIS (*(volatile uint32_t *)(ETH_BASE + 0x00))
#define MAC_RCTL (*(volatile uint32_t *)(ETH_BASE + 0x08))
#define MAC_NP (*(volatile uint32_t *)(ETH_BASE + 0x34))

extern void eth_init(void);
extern void eth_poll(void);
extern void eth_send(const uint8_t *data, uint32_t len);
extern uint32_t eth_receive(uint8_t *buf, uint32_t max_len);

#endif /* STELLARIS_ETH_H */
