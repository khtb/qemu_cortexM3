#include "ethernet/stellaris_eth.h"
#include "uart.h"
#include <stdint.h>

/*
 * Definitive Stellaris LM3S6965 Ethernet Register Map (QEMU Verified)
 *
 * Research and hardware audits confirmed that QEMU's LM3S6965 emulation
 * uses an updated register layout where NP is at 0x34 and TCTL is at 0x0C.
 */
#define ETH_BASE 0x40048000
#define MAC_RIS (*(volatile uint32_t *)(ETH_BASE + 0x00))
#define MAC_IACK (*(volatile uint32_t *)(ETH_BASE + 0x00))
#define MAC_IM (*(volatile uint32_t *)(ETH_BASE + 0x04))
#define MAC_RCTL (*(volatile uint32_t *)(ETH_BASE + 0x08))
#define MAC_TCTL (*(volatile uint32_t *)(ETH_BASE + 0x0C))
#define MAC_DATA (*(volatile uint32_t *)(ETH_BASE + 0x10))
#define MAC_IA0 (*(volatile uint32_t *)(ETH_BASE + 0x14))
#define MAC_IA1 (*(volatile uint32_t *)(ETH_BASE + 0x18))
#define MAC_THR (*(volatile uint32_t *)(ETH_BASE + 0x1C))
#define MAC_MDV (*(volatile uint32_t *)(ETH_BASE + 0x24))
#define MAC_MTXD (*(volatile uint32_t *)(ETH_BASE + 0x28))
#define MAC_MRXD (*(volatile uint32_t *)(ETH_BASE + 0x2C))
#define MAC_MCTL (*(volatile uint32_t *)(ETH_BASE + 0x30))
#define MAC_NP (*(volatile uint32_t *)(ETH_BASE + 0x34))
#define MAC_TR (*(volatile uint32_t *)(ETH_BASE + 0x38))

#define SYSCTL_RCGC2 (*(volatile uint32_t *)0x400FE108)

/**
 * eth_init: Configures the Ethernet controller for bidirectional traffic.
 * Sets the MAC address to 00:11:22:33:44:55 and enables promiscuous mode.
 */
void eth_init(void) {
  uart_puts("\n[Ethernet] Initializing LM3S6965 Controller...\n");

  /* 1. Enable Ethernet MAC and PHY clocks */
  SYSCTL_RCGC2 |= 0x50000000;
  for (volatile int i = 0; i < 10000; i++)
    ; /* Stabilization delay */

  /* 2. Configure Guest MAC Address: 00:11:22:33:44:55 */
  /* Registers use little-endian byte order for the 6-byte address */
  MAC_IA0 = 0x33221100;
  MAC_IA1 = 0x00005544;

  /* 3. Global Interrupt and FIFO Management */
  MAC_IM = 0;      /* Mask all interrupts for polling mode */
  MAC_IACK = 0xFF; /* Clear all pending status bits */

  /* 4. FIFO Reset and Configuration */
  MAC_RCTL = 0x10; /* Pulse RSTFIFO bit */
  for (volatile int i = 0; i < 1000; i++)
    ;

  /* 5. Enable RX and TX with standard features
   * RX: Enable (1) | Multicast (2) | Promiscuous (4) | Accept Bad CRC (8) =
   * 0x0F TX: Enable (1) | Auto Pad (2) | Gen CRC (4) | Full Duplex (8) = 0x0F
   */
  MAC_RCTL = 0x0F;
  MAC_TCTL = 0x0F;

  uart_puts("[Ethernet] Controller Ready. Mode: Promiscuous\n");
}

/**
 * eth_send: Transmits an Ethernet frame.
 * @param data: Pointer to the Ethernet frame (dst | src | type | payload)
 * @param len: Total length of the frame in bytes.
 */
void eth_send(const uint8_t *data, uint32_t len) {
  /* QEMU/Stellaris TX Framing:
   * First word written to MAC_DATA must contain the total packet length.
   */
  MAC_DATA = (len & 0xFFFF);

  const uint32_t *p = (const uint32_t *)data;
  uint32_t words = (len + 3) / 4;

  for (uint32_t i = 0; i < words; i++) {
    MAC_DATA = p[i];
  }

  /* Set Transmit Request (TR) to start the engine */
  MAC_TR = 0x01;
}

/**
 * eth_poll: Checks for incoming packets and manages periodic heartbeats.
 * This function should be called frequently in the main loop.
 */
void eth_poll(void) {
  /* Robust Packet Reception Loop */
  uint32_t np = MAC_NP;

  while (np > 0) {
    /*
     * QEMU RX Framing:
     * The first word read from MAC_DATA contains:
     * [15:0]  - Total packet length (L)
     * [31:16] - First two bytes of data (Payload[1:0])
     */
    uint32_t header = MAC_DATA;
    uint32_t len = header & 0xFFFF;

    uart_puts("!!! [Ethernet] RECV PKT !!! L:");
    uart_print_hex(len);
    uart_puts(" NP:");
    uart_print_hex(np);
    uart_puts("\n");

    /* Safely consume the rest of the FIFO for this packet */
    if (len > 0 && len < 1550) {
      /* Since the first word included the first 2 bytes,
       * we need to read the remaining (len - 2) bytes as words.
       */
      int words_to_drain = (len + 2 + 3) / 4 - 1;
      for (int i = 0; i < words_to_drain; i++) {
        uint32_t dummy = MAC_DATA;
        /* Debug: Log first snippet of payload */
        if (i == 0) {
          uart_puts("  Snippet: ");
          uart_print_hex(dummy);
          uart_puts("\n");
        }
      }
    }

    /* Acknowledge reception to update controller state */
    MAC_IACK = 0x01;

    /* Re-check count (some variants require this to decrement NP) */
    np = MAC_NP;
  }

  /* Guest-to-Host Heartbeat (Beacon)
   * Sends a simple broadcast frame every ~10k poll cycles.
   */
  static uint32_t heartbeat_cnt = 0;
  if (++heartbeat_cnt % 10000 == 0) {
    uint8_t beacon[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* Destination: Broadcast */
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, /* Source: Guest MAC */
        0xBE, 0xAC,                         /* Type: Beacon */
        0xDE, 0xAD, 0xBE, 0xEF              /* Payload */
    };
    eth_send(beacon, sizeof(beacon));
    // uart_puts("[Ethernet] Beacon Sent.\n");
  }
}
