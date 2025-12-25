/*
###############################################################################
#        _   __  _   _   _____   ____                           
#       | | / / | | | | (_   _) |  _ \                          
#       | |/ /  | |_| |   | |   | |_) )                         
#       |   <   |  _  |   | |   |  _ (                          
#       | |\ \  | | | |   | |   | |_) )                        
#       |_| \_\ |_| |_|   |_|   |____/                         
#                                                               
###############################################################################
******************************************************************************
 * @file uart.c
 * Author: KHTB 
 * @brief Description of the header file
******************************************************************************/
#include "uart.h"



volatile char rxBuffer[RX_BUF_SIZE];
volatile uint32_t rxHead = 0, rxTail = 0;
volatile uint32_t txHead = 0, txTail = 0;


void uart_puts(const char *s);
void uart_putc(char c);


void uart_init(void)
{
    UART0->CTL = 0;

    UART0->IBRD = 43;
    UART0->FBRD = 26;

    UART0->LCRH =(0x3 << 5); /* 8 bits */
    UART0->CTL = (1<<0) | (1<<8 ) | (1 <<9 ); /* enable , TXE and RXE */
        // Enable interrupts
    UART0->IM |= (1 << 4) | (1 << 5);  // RXIM + TXIM
    NVIC_EnableIRQ(UART0_IRQn);
}

void uart_irq(void)
{
    uint32_t status = UART0->MIS;
    // RX interrupt
    if (status & (1 << 4)) // RXIM
    {
        uint32_t next = (rxHead +1) %RX_BUF_SIZE;
        char c = UART0->DR;
        if(next != rxTail)
        {
            rxBuffer[rxHead] = c;
            rxHead = next;
        }
        if (c =='\r' || c =='\n')
        {
            uart_puts("\r\n");
        }
        if (c == 0x08 || c ==0x7f)
        {
            uart_puts("\b \b");
        }
        else
        {
            uart_putc(c);
        }

        UART0->ICR = (1 << 4); // Clear RX interrupt
    }

    // // TX interrupt
    if (status & (1 << 5)) // TXIM
    {
    //     // If something to send
    //     if (txTail != txHead)
    //     {
    //         UART0->DR = rxBuffer[txTail++];
    //         txTail &= 63;
    //     }
    //     else
    //     {
    //         // No more data → disable TX interrupt
    //         UART0->IM &= ~(1 << 5);
    //     }

        UART0->ICR = (1 << 5);
    }
}

void uart_putc(char c)
{
    // Wait if TX FIFO is full
    while (UART0->FR & (1 << 5))
        ; // TXFF

    // Write directly to UART data register
    UART0->DR = c;

    // Enable TX interrupt so ISR gets called when FIFO gets empty
    UART0->IM |= (1 << 5); // TXIM
}

void uart_puts(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}


void uart_print(const char *s)
{
    uart_puts(s);
}

// Simple getchar using buffer
int uart_getchar(void)
{
    if (rxHead == rxTail)
        return -1; // no data

    char c = rxBuffer[rxTail];
    rxTail = (rxTail + 1) % RX_BUF_SIZE;
    return c;
}


int uart_readLine(char * line, int max_len)
{
    static int len = 0;
    int c;
    while ((c = uart_getchar()) != -1)
    {
        if (c == '\n' || c == '\r')
        {
            line[len] = '\0';
            int result = len;
            len = 0;
            return result;
        }
        else if (c == '\b' || c == 0x7F)
        {
            if (len > 0)
            {
                len--;
            }
        }
        else if (len < max_len - 1)
        {
            line[len++] = c;
            line[len] = '\0';
        }
        else
        {
            // error
        }
    }
}
void uart_print_hex(uint32_t v) {
  const char hex[] = "0123456789ABCDEF";
  uart_puts("0x");
  for (int i = 7; i >= 0; i--) {
    uart_putc(hex[(v >> (i * 4)) & 0xF]);
  }
}
