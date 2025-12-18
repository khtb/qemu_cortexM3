#ifndef _UART_H_
#define _UART_H_


#if 0

/* ---- CMSDK UART0 on MPS2-AN386 ---- */
#define UART0_BASE 0x40004000u /* APB UART0 base */
#define UART0_IRQn   0  // IRQ number in QEMU for UART0

typedef struct {
    volatile uint32_t DATA;      /* 0x00 */
    volatile uint32_t STATE;     /* 0x04 */
    volatile uint32_t CTRL;      /* 0x08 */
    volatile uint32_t INTSTATUS; /* 0x0C (also INTCLEAR on write) */
    volatile uint32_t BAUDDIV;   /* 0x10 */
} CMSDK_UART;
#endif

typedef struct
{
    volatile uint32_t DR;        /* 0x000 Data */
    volatile uint32_t RSR_ECR;   /* 0x004 Status / Error Clear */
    uint32_t RESERVED0[4];       /* 0x008–0x017 */
    volatile uint32_t FR;        /* 0x018 Flag Register */
    uint32_t RESERVED1;          /* 0x01C */
    volatile uint32_t ILPR;      /* 0x020 IrDA Low-Power */
    volatile uint32_t IBRD;      /* 0x024 Integer Baud */
    volatile uint32_t FBRD;      /* 0x028 Fractional Baud */
    volatile uint32_t LCRH;      /* 0x02C Line Control */
    volatile uint32_t CTL;       /* 0x030 Control */
    volatile uint32_t IFLS;      /* 0x034 */
    volatile uint32_t IM;        /* 0x038 Interrupt Mask */
    volatile uint32_t RIS;       /* 0x03C Raw Int Status */
    volatile uint32_t MIS;       /* 0x040 Masked Int Status */
    volatile uint32_t ICR;       /* 0x044 Interrupt Clear */
    volatile uint32_t DMACTL;    /* 0x048 DMA Control */
} UART_TypeDef;



#define UART0_BASE   0x4000C000
#define UART1_BASE   0x4000D000
#define UART2_BASE   0x4000E000

#define UART0 ((UART_TypeDef *)UART0_BASE)
#define UART1 ((UART_TypeDef *)UART1_BASE)
#define UART2 ((UART_TypeDef *)UART2_BASE)

#define UART_FR_TXFE   (1 << 7)  // TX FIFO empty
#define UART_FR_RXFF   (1 << 6)  // RX FIFO full
#define UART_FR_TXFF   (1 << 5)  // TX FIFO full
#define UART_FR_RXFE   (1 << 4)  // RX FIFO empty
#define UART_FR_BUSY   (1 << 3)  // UART is busy






// NVIC registers (simple version)
#define NVIC_ISER0       (*((volatile uint32_t *)0xE000E100))
#define NVIC_EnableIRQ(irq)   (NVIC_ISER0 = (1 << (irq)))


extern void uart_init(void);
extern void uart_irq(void);
extern void uart_puts(const char* s);
extern void uart_print(const char* s);
extern int uart_getchar(void);
extern int uart_readLine(char * line, int max_len);

#endif