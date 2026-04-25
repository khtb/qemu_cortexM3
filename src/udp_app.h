#ifndef UDP_APP_H
#define UDP_APP_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t header;       // Magic word, e.g., 0xDEADBEEF
    uint32_t msg_id;       // Message sequence ID
    uint32_t cmd;          // Command ID
    uint32_t payload_len;  // Length of valid data in the payload array
    uint8_t  payload[1184]; // Padding/Data to reach exactly 1200 bytes
} comm_packet_t;
#pragma pack(pop)

void udp_app_init(void);

#endif /* UDP_APP_H */
