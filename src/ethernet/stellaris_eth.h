#ifndef STELLARIS_ETH_H
#define STELLARIS_ETH_H

#include <stdint.h>

extern void eth_init(void);
extern void eth_poll(void);
extern void eth_send(const uint8_t *data, uint32_t len);


#endif /* STELLARIS_ETH_H */
