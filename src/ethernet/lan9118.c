#if 0
#include "ethernet/lan9118.h"
#include "uart/uart.h" /* For debug printing */

#define LAN9118_BASE 0x40200000
#define LAN9118_RX_DATA_FIFO (*(volatile uint32_t *)(LAN9118_BASE + 0x00))
#define LAN9118_TX_DATA_FIFO (*(volatile uint32_t *)(LAN9118_BASE + 0x20))
#define LAN9118_RX_STATUS_FIFO (*(volatile uint32_t *)(LAN9118_BASE + 0x40))
#define LAN9118_RX_STATUS_PEEK (*(volatile uint32_t *)(LAN9118_BASE + 0x44))
#define LAN9118_TX_STATUS_FIFO (*(volatile uint32_t *)(LAN9118_BASE + 0x48))
#define LAN9118_ID_REV (*(volatile uint32_t *)(LAN9118_BASE + 0x50))
#define LAN9118_IRQ_CFG (*(volatile uint32_t *)(LAN9118_BASE + 0x54))
#define LAN9118_INT_STS (*(volatile uint32_t *)(LAN9118_BASE + 0x58))
#define LAN9118_INT_EN (*(volatile uint32_t *)(LAN9118_BASE + 0x5C))
#define LAN9118_BYTE_TEST (*(volatile uint32_t *)(LAN9118_BASE + 0x64))
#define LAN9118_FIFO_INT (*(volatile uint32_t *)(LAN9118_BASE + 0x68))
#define LAN9118_RX_CFG (*(volatile uint32_t *)(LAN9118_BASE + 0x6C))
#define LAN9118_TX_CFG (*(volatile uint32_t *)(LAN9118_BASE + 0x70))
#define LAN9118_HW_CFG (*(volatile uint32_t *)(LAN9118_BASE + 0x74))
#define LAN9118_RX_DP_CTRL (*(volatile uint32_t *)(LAN9118_BASE + 0x78))
#define LAN9118_RX_FIFO_INF (*(volatile uint32_t *)(LAN9118_BASE + 0x7C))
#define LAN9118_TX_FIFO_INF (*(volatile uint32_t *)(LAN9118_BASE + 0x80))
#define LAN9118_PMT_CTRL (*(volatile uint32_t *)(LAN9118_BASE + 0x84))
#define LAN9118_GPIO_CFG (*(volatile uint32_t *)(LAN9118_BASE + 0x88))
#define LAN9118_GPT_CFG (*(volatile uint32_t *)(LAN9118_BASE + 0x8C))
#define LAN9118_GPT_CNT (*(volatile uint32_t *)(LAN9118_BASE + 0x90))
#define LAN9118_MAC_CSR_CMD (*(volatile uint32_t *)(LAN9118_BASE + 0xA4))
#define LAN9118_MAC_CSR_DATA (*(volatile uint32_t *)(LAN9118_BASE + 0xA8))
#define LAN9118_AFC_CFG (*(volatile uint32_t *)(LAN9118_BASE + 0xAC))

/* Standard LAN9118 ID */
#define LAN9118_ID_VAL 0x01180000

/* Ethernet Driver Utils */
static void delay(volatile int count) {
  while (count--)
    __asm("nop");
}

static uint32_t mac_read_csr(uint8_t reg) {
  while (LAN9118_MAC_CSR_CMD & 0x80000000)
    ;                                                 // Wait for busy
  LAN9118_MAC_CSR_CMD = 0x80000000 | (1 << 30) | reg; // Read command
  while (LAN9118_MAC_CSR_CMD & 0x80000000)
    ; // Wait for busy
  return LAN9118_MAC_CSR_DATA;
}

static void mac_write_csr(uint8_t reg, uint32_t val) {
  while (LAN9118_MAC_CSR_CMD & 0x80000000)
    ; // Wait for busy
  LAN9118_MAC_CSR_DATA = val;
  LAN9118_MAC_CSR_CMD = 0x80000000 | reg; // Write command
  while (LAN9118_MAC_CSR_CMD & 0x80000000)
    ; // Wait for busy
}

void eth_init(void) {
  uart_puts("Initializing LAN9118...\n");

  /* Check ID */
  uint32_t id = LAN9118_ID_REV;
  uart_puts("LAN9118 ID: ");
  uart_print_hex(id);
  uart_puts("\n");

  if ((id & 0xFFFF0000) != LAN9118_ID_VAL &&
      (id & 0xFFFF0000) != 0x92180000 /* LAN9218 check just in case */) {
    uart_puts("WARNING: Unexpected Device ID.\n");
  }

  /* Soft Reset */
  LAN9118_HW_CFG = 0x00000001;
  delay(1000);
  while (LAN9118_HW_CFG & 0x01)
    ; // Wait for reset to clear

  /* Configure GPIO/AFC as per standard defaults or requirements */
  LAN9118_AFC_CFG = 0x006E3740;

  /* Enable MAC TX/RX */
  uint32_t mac_cr = mac_read_csr(1); // MAC_CR
  mac_cr |= (1 << 3) | (1 << 2);     // TXEN, RXEN
  mac_write_csr(1, mac_cr);

  /* Enable TX/RX in HW_CFG/TX_CFG/RX_CFG */
  LAN9118_TX_CFG = 0x00000002; // TX_ON
  LAN9118_RX_CFG =
      0x00000000; // Recv offset 2 bytes (alignment) -> QEMU simpler? 0

  uart_puts("LAN9118 Initialized.\n");
}

/* Polling for packets */
void eth_poll(void) {
  uint32_t rx_fifo_inf = LAN9118_RX_FIFO_INF;
  if (rx_fifo_inf & 0x00FF0000) { // Bytes available check? or use Status FIFO
    uint32_t status_levels = LAN9118_RX_FIFO_INF & 0xFF;
    if (status_levels > 0) {
      uint32_t rx_status = LAN9118_RX_STATUS_FIFO;
      uint32_t pkt_len = (rx_status >> 16) & 0x3FFF;

      uart_puts("Packet Received! Length: ");
      uart_print_hex(pkt_len);
      uart_puts("\n");

      if (pkt_len > 0) {
        /* Read packet data */
        uint32_t words =
            (pkt_len + 3 + 2) / 4; // +2 for default offset padding if
                                   // any, usually 0 in simplified QEMU
        /* Actually QEMU/Lan9118 usually doesn't add padding unless
         * RX_CFG */

        uart_puts("Data: ");
        for (uint32_t i = 0; i < words; i++) {
          uint32_t d = LAN9118_RX_DATA_FIFO;
          if (i < 4) { // Print first few words
            uart_print_hex(d);
            uart_putc(' ');
          }
        }
        uart_puts("...\n");
      }
    }
  }
}
#endif