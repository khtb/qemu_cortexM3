#ifndef ETH_LOG_H
#define ETH_LOG_H

#include <stdint.h>

/**
 * eth_log_init: Initializes the Ethernet logger (and driver if needed).
 */
void eth_log_init(void);

/**
 * eth_printf: Formats a string and transmits it as an Ethernet frame.
 * Uses EtherType 0x88B5.
 * @param fmt: Printf-style format string.
 */
void eth_printf(const char *fmt, ...);

#endif
