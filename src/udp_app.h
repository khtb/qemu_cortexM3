#ifndef UDP_APP_H
#define UDP_APP_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint32_t header;       // Magic word, e.g., 0xDEADBEEF
    uint32_t msg_id;       // Message sequence ID
    uint32_t cmd;          // Command ID
    uint32_t payload_len;  // Length of valid data in the payload array
    uint8_t  payload[1180]; // Padding/Data to reach exactly 1196 bytes
    uint32_t eof_marker;   // End of frame marker, e.g., 0xE0F0E0F0
} comm_packet_t;
#pragma pack(pop)

#define CMD_ECHO         0x1001
#define CMD_GET_3_FRAMES 0x1002
#define EOF_MARKER_VAL   0xE0F0E0F0

void udp_app_init(void);

#endif /* UDP_APP_H */
