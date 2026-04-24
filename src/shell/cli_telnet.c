#include "FreeRTOS.h"
#include "lwip/tcp.h"
#include "shell.h"
#include "uart.h"
#include <string.h>

#define TELNET_PORT 23

static struct tcp_pcb *telnet_pcb = NULL;
static struct tcp_pcb *active_pcb = NULL;

void telnet_puts(const char *str)
{
    if (active_pcb)
    {
        err_t err = tcp_write(active_pcb, str, strlen(str), TCP_WRITE_FLAG_COPY);
        err_t err2 = tcp_output(active_pcb);
        // Print the error codes to uart so we know if it failed!
        uart_printf("[telnet_puts] writing %d bytes, err=%d, out_err=%d\r\n", strlen(str), err, err2);
    }
    else
    {
        uart_printf("[telnet_puts] active_pcb is NULL\r\n");
    }
}

static void telnet_write(const char *str)
{
    telnet_puts(str);
}

/* Telnet Line Editing similar to UART */
#define MAX_CMD_LEN 64
#define HISTORY_DEPTH 5
static char telnet_history[HISTORY_DEPTH][MAX_CMD_LEN];
static int history_count = 0;
static int history_head = 0;

static void telnet_add_history(const char *cmd)
{
    if (cmd[0] == '\0')
        return;
    int prev_idx = (history_head - 1 + HISTORY_DEPTH) % HISTORY_DEPTH;
    if (history_count > 0 && strcmp(telnet_history[prev_idx], cmd) == 0)
        return;

    strncpy(telnet_history[history_head], cmd, MAX_CMD_LEN - 1);
    telnet_history[history_head][MAX_CMD_LEN - 1] = '\0';
    history_head = (history_head + 1) % HISTORY_DEPTH;
    if (history_count < HISTORY_DEPTH)
        history_count++;
}

static struct {
    char cmd_buf[MAX_CMD_LEN];
    int cmd_len;
    int esc_state;
    int history_pos;
} telnet_state;

static void telnet_conn_err(void *arg, err_t err)
{
    LWIP_UNUSED_ARG(arg);
    active_pcb = NULL;
    uart_puts("[Telnet] Connection Exception/Error.\n");
}

static err_t telnet_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (p == NULL)
    {
        /* Connection closed by remote host */
        active_pcb = NULL;
        tcp_arg(tpcb, NULL);
        tcp_recv(tpcb, NULL);
        tcp_err(tpcb, NULL);
        tcp_close(tpcb);
        uart_puts("[Telnet] Connection Closed.\n");
        return ERR_OK;
    }

    if (err != ERR_OK) {
        if (p != NULL) {
            pbuf_free(p);
        }
        return err;
    }

    tcp_recved(tpcb, p->tot_len);

    struct pbuf *q;
    for (q = p; q != NULL; q = q->next)
    {
        uint8_t *cdata = (uint8_t *)q->payload;
        for (int i = 0; i < q->len; i++)
        {
            uint8_t c = cdata[i];

            if (c == 0xFF)
            {
                i += 2;
                continue;
            }

            if (telnet_state.esc_state == 1)
            {
                if (c == '[' || c == 'O')
                    telnet_state.esc_state = 2;
                else
                    telnet_state.esc_state = 0;
            }
            else if (telnet_state.esc_state == 2)
            {
                if (c == 'A') /* Up Arrow */
                {
                    if (history_count > 0 && telnet_state.history_pos < history_count)
                    {
                        telnet_state.history_pos++;
                        int idx = (history_head - telnet_state.history_pos + HISTORY_DEPTH) % HISTORY_DEPTH;
                        while (telnet_state.cmd_len > 0)
                        {
                            telnet_write("\b \b");
                            telnet_state.cmd_len--;
                        }

                        strncpy(telnet_state.cmd_buf, telnet_history[idx], MAX_CMD_LEN - 1);
                        telnet_state.cmd_buf[MAX_CMD_LEN - 1] = '\0';
                        telnet_state.cmd_len = strlen(telnet_state.cmd_buf);
                        telnet_write(telnet_state.cmd_buf);
                    }
                }
                else if (c == 'B') /* Down Arrow */
                {
                    if (telnet_state.history_pos > 0)
                    {
                        telnet_state.history_pos--;
                        while (telnet_state.cmd_len > 0)
                        {
                            telnet_write("\b \b");
                            telnet_state.cmd_len--;
                        }

                        if (telnet_state.history_pos > 0)
                        {
                            int idx = (history_head - telnet_state.history_pos + HISTORY_DEPTH) % HISTORY_DEPTH;
                            strncpy(telnet_state.cmd_buf, telnet_history[idx], MAX_CMD_LEN - 1);
                            telnet_state.cmd_buf[MAX_CMD_LEN - 1] = '\0';
                            telnet_state.cmd_len = strlen(telnet_state.cmd_buf);
                            telnet_write(telnet_state.cmd_buf);
                        }
                        else
                        {
                            telnet_state.cmd_buf[0] = '\0';
                            telnet_state.cmd_len = 0;
                        }
                    }
                }
                telnet_state.esc_state = 0;
            }
            else /* Normal State */
            {
                if (c == 0x1B) /* ESC */
                {
                    telnet_state.esc_state = 1;
                }
                else if (c == '\r' || c == '\n')
                {
                    if (telnet_state.cmd_len > 0)
                    {
                        telnet_state.cmd_buf[telnet_state.cmd_len] = '\0';
                        telnet_write("\r\n");
                        telnet_add_history(telnet_state.cmd_buf);

                        shell_process(telnet_state.cmd_buf, telnet_write);

                        telnet_state.cmd_len = 0;
                        telnet_state.history_pos = 0;
                    }
                    else
                    {
                        telnet_write("\r\n");
                    }
                    telnet_write("\r\n> ");
                }
                else if (c == 0x08 || c == 0x7F) /* Backspace */
                {
                    if (telnet_state.cmd_len > 0)
                    {
                        telnet_state.cmd_len--;
                        telnet_write("\b \b");
                    }
                }
                else if (c == 0x09) /* TAB */
                {
                    int old_len = telnet_state.cmd_len;
                    if (shell_autocomplete(telnet_state.cmd_buf, &telnet_state.cmd_len, MAX_CMD_LEN))
                    {
                        if (telnet_state.cmd_len > old_len)
                        {
                            telnet_write(&telnet_state.cmd_buf[old_len]);
                        }
                    }
                }
                else if (c >= 32 && c <= 126)
                {
                    if (telnet_state.cmd_len < MAX_CMD_LEN - 1)
                    {
                        telnet_state.cmd_buf[telnet_state.cmd_len++] = c;
                        char echo[2] = {c, 0};
                        telnet_write(echo);
                    }
                }
            }
        }
    }
    pbuf_free(p);
    return ERR_OK;
}

static err_t telnet_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    LWIP_UNUSED_ARG(arg);

    if (err != ERR_OK || newpcb == NULL) {
        return ERR_VAL;
    }

    if (active_pcb != NULL) {
        uart_puts("[Telnet] Rejecting new connection, already active.\n");
        return ERR_ABRT; // Reject
    }

    uart_puts("[Telnet] Connection Accepted.\n");

    active_pcb = newpcb;
    telnet_state.cmd_len = 0;
    telnet_state.esc_state = 0;
    telnet_state.history_pos = 0;

    /* Setup callbacks */
    tcp_arg(newpcb, NULL);
    tcp_recv(newpcb, telnet_recv);
    tcp_err(newpcb, telnet_conn_err);

    /* Telnet Negotiation: WILL ECHO, WILL SUPPRESS_GO_AHEAD */
    static const uint8_t telnet_negotiation[] = {
        0xFF, 0xFB, 0x01, /* IAC WILL ECHO */
        0xFF, 0xFB, 0x03  /* IAC WILL SGA */
    };
    tcp_write(newpcb, telnet_negotiation, sizeof(telnet_negotiation), TCP_WRITE_FLAG_COPY);

    /* Send Welcome Message */
    telnet_write("\r\nWelcome to CLI Telnet Server\r\n> ");
    tcp_output(newpcb);

    return ERR_OK;
}

void telnet_init(void)
{
    telnet_pcb = tcp_new();
    if (telnet_pcb != NULL)
    {
        err_t err = tcp_bind(telnet_pcb, IP_ADDR_ANY, TELNET_PORT);
        if (err == ERR_OK)
        {
            telnet_pcb = tcp_listen(telnet_pcb);
            tcp_accept(telnet_pcb, telnet_accept);
            uart_puts("[Telnet] Listening on port 23 (Raw API)...\n");
        }
        else
        {
            uart_puts("[Telnet] Bind failed.\n");
            tcp_close(telnet_pcb);
        }
    }
    else
    {
        uart_puts("[Telnet] tcp_new failed.\n");
    }
}
