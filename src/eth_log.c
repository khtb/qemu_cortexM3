#include "eth_log.h"
#include "ethernet/stellaris_eth.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// EtherType for Logging (Custom)
#define ETH_P_LOG 0x88B5

// Destination MAC: Broadcast
static const uint8_t dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
// Source MAC: QEMU Default for Stellaris
static const uint8_t src_mac[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

void eth_log_init(void) { eth_init(); }

void eth_printf(const char *fmt, ...)
{
    uint8_t frame[1514];
    uint32_t payload_len;
    va_list args;

    // Header construction
    memcpy(&frame[0], dst_mac, 6);
    memcpy(&frame[6], src_mac, 6);
    frame[12] = (ETH_P_LOG >> 8) & 0xFF;
    frame[13] = ETH_P_LOG & 0xFF;

    // Payload construction
    va_start(args, fmt);
    // Offset 14 for Ethernet Header
    payload_len = vsnprintf((char *)&frame[14], sizeof(frame) - 14, fmt, args);
    va_end(args);

    if (payload_len > 0)
    {
        // Ensure string is null-terminated or just send existing length?
        // eth_logger expects string, let's send text.
        // If vsnprintf returns len without null, it might be truncated.
        // We send the formatted string. The logger on host handles the rest.

        // Total frame len = Header (14) + Payload
        eth_send(frame, 14 + payload_len);
    }
}
