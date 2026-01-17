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
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// Forward declarations
void uart_putc(char c);

static uart_autocomplete_cb_t autocomplete_cb = NULL;

void uart_set_autocomplete_cb(uart_autocomplete_cb_t cb) { autocomplete_cb = cb; }

// --- Line Editing Globals (Managed in IRQ) ---
#define MAX_CMD_LEN 64
static char cmd_buf[MAX_CMD_LEN];
static int cmd_len = 0;
static volatile int line_ready = 0;

// Escape Sequence State Machine
// 0: Normal
// 1: ESC received (0x1B)
// 2: '[' or 'O' received
/* ANSI Escape Codes */

static int esc_state = 0;

// History
#define HISTORY_DEPTH 5
static char history[HISTORY_DEPTH][MAX_CMD_LEN];
static int history_count = 0;
static int history_head = 0;
static int history_pos = 0; // 0=current, 1..N=older

static void uart_add_history_irq(const char *cmd)
{
    if (cmd[0] == '\0')
        return;

    // Check duplicate
    int prev_idx = (history_head - 1 + HISTORY_DEPTH) % HISTORY_DEPTH;
    if (history_count > 0 && strcmp(history[prev_idx], cmd) == 0)
    {
        return;
    }

    strncpy(history[history_head], cmd, MAX_CMD_LEN - 1);
    history[history_head][MAX_CMD_LEN - 1] = '\0';
    history_head = (history_head + 1) % HISTORY_DEPTH;
    if (history_count < HISTORY_DEPTH)
        history_count++;
}

void uart_init(void)
{
    UART0->CTL = 0;
    UART0->IBRD = 43;
    UART0->FBRD = 26;
    UART0->LCRH = (0x3 << 5) | (1 << 4);         /* 8 bits, FIFO Enable */
    UART0->CTL = (1 << 0) | (1 << 8) | (1 << 9); /* enable , TXE and RXE */
    // Enable interrupts: RXIM(4) + TXIM(5) + RTIM(6)
    UART0->IM |= (1 << 4) | (1 << 5) | (1 << 6);
    NVIC_EnableIRQ(UART0_IRQn);
    cmd_len = 0;
    line_ready = 0;
    esc_state = 0;
}

void uart_irq(void)
{
    uint32_t status = UART0->MIS;

    // --- RX Interrupt (Bit 4) OR Receive Timeout (Bit 6) ---
    if (status & ((1 << 4) | (1 << 6)))
    {
        // Loop while RX FIFO is NOT empty
        while (!(UART0->FR & UART_FR_RXFE))
        {
            char c = UART0->DR;

            // If line is already ready, drop chars until it's consumed
            if (line_ready)
            {
                continue;
            }

            // --- Escape Sequence Machine ---
            if (esc_state == 1)
            {
                if (c == '[' || c == 'O')
                {
                    esc_state = 2;
                }
                else
                {
                    esc_state = 0; // Abort/Unknown
                }
            }
            else if (esc_state == 2)
            {
                // Expecting 'A' (Up) or 'B' (Down) usually
                if (c == 'A')
                { // Up
                    if (history_count > 0 && history_pos < history_count)
                    {
                        history_pos++;
                        int idx = (history_head - history_pos + HISTORY_DEPTH) % HISTORY_DEPTH;
                        // Clear line
                        while (cmd_len > 0)
                        {
                            uart_puts("\b \b");
                            cmd_len--;
                        }
                        // Copy history
                        strncpy(cmd_buf, history[idx], MAX_CMD_LEN - 1);
                        cmd_buf[MAX_CMD_LEN - 1] = '\0';
                        cmd_len = strlen(cmd_buf);
                        uart_puts(cmd_buf);
                    }
                }
                else if (c == 'B')
                { // Down
                    if (history_pos > 0)
                    {
                        history_pos--;
                        // Clear line
                        while (cmd_len > 0)
                        {
                            uart_puts("\b \b");
                            cmd_len--;
                        }
                        if (history_pos > 0)
                        {
                            int idx = (history_head - history_pos + HISTORY_DEPTH) % HISTORY_DEPTH;
                            strncpy(cmd_buf, history[idx], MAX_CMD_LEN - 1);
                            cmd_buf[MAX_CMD_LEN - 1] = '\0';
                            cmd_len = strlen(cmd_buf);
                            uart_puts(cmd_buf);
                        }
                        else
                        {
                            // Empty
                            cmd_buf[0] = '\0';
                            cmd_len = 0;
                        }
                    }
                }
                esc_state = 0;
            }
            else
            {
                // Normal State (esc_state == 0)
                if (c == 0x1B)
                {
                    esc_state = 1;
                }
                else if (c == '\r' || c == '\n')
                {
                    cmd_buf[cmd_len] = '\0';
                    uart_puts("\r\n");
                    uart_add_history_irq(cmd_buf);
                    line_ready = 1;
                    history_pos = 0; // Reset history for next line
                }
                else if (c == 0x08 || c == 0x7F)
                { // Backspace
                    if (cmd_len > 0)
                    {
                        cmd_len--;
                        uart_puts("\b \b");
                    }
                }
                else if (c == 0x09)
                { // TAB
                    if (autocomplete_cb && cmd_len > 0)
                    {
                        int old_len = cmd_len;
                        autocomplete_cb(cmd_buf, &cmd_len, MAX_CMD_LEN);
                        if (cmd_len > old_len)
                        {
                            uart_puts(&cmd_buf[old_len]); // Echo appended
                                                          // part
                        }
                    }
                }
                else if (c >= 32 && c <= 126)
                {
                    if (cmd_len < MAX_CMD_LEN - 1)
                    {
                        cmd_buf[cmd_len++] = c;
                        uart_putc(c); // Echo
                    }
                }
            }
        } // while (!RXFE)

        // Clear RX (4) and RTI (6)
        UART0->ICR = (1 << 4) | (1 << 6);
    }

    // --- TX Interrupt ---
    if (status & (1 << 5))
    {
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

void uart_print(const char *s) { uart_puts(s); }

void uart_log_and_refresh(const char *log, const char *prompt)
{
    // Clear the current line: \r (start) + \x1B[2K (erase line)
    uart_puts("\r\x1B[2K");

    // Print the log message (ensure ends with newline)
    uart_puts(log);
    uart_puts("\r\n");

    // Reprint the prompt
    uart_puts(prompt);

    // Reprint the current edited line
    for (int i = 0; i < cmd_len; i++)
    {
        uart_putc(cmd_buf[i]);
    }
}

// Just returns empty invalid if called directly, or could return last line
int uart_getchar(void) { return -1; }

int uart_readLine(char *line, int max_len)
{
    if (!line_ready)
    {
        return -1;
    }

    // Copy the prepared buffer
    strncpy(line, cmd_buf, max_len - 1);
    line[max_len - 1] = '\0';

    int ret_len = strlen(line);

    // Reset for next command
    cmd_len = 0;
    line_ready = 0;

    return ret_len;
}

void uart_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_puts(buf);
}

/* print value in hex , to be optimized */
void uart_print_hex(uint32_t v)
{
    const char hex[] = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 7; i >= 0; i--)
    {
        uart_putc(hex[(v >> (i * 4)) & 0xF]);
    }
}
