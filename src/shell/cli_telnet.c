#include "FreeRTOS.h"
#include "lwip/api.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "shell.h"
#include "task.h"
#include "uart.h"
#include <string.h>

#define TELNET_PORT 23
#define TELNET_THREAD_STACKSIZE 1024
#define TELNET_THREAD_PRIO 2

static struct netconn *active_conn = NULL;

static void telnet_write(const char *str)
{
    if (active_conn)
    {
        netconn_write(active_conn, str, strlen(str), NETCONN_COPY);
    }
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

static void telnet_process_connection(struct netconn *conn)
{
    struct netbuf *buf;
    void *data;
    u16_t len;
    err_t err;

    /* Line Editor State */
    char cmd_buf[MAX_CMD_LEN];
    int cmd_len = 0;
    int esc_state = 0;
    int history_pos = 0;

    active_conn = conn;

    /* Telnet Negotiation: WILL ECHO, WILL SUPPRESS_GO_AHEAD */
    static const uint8_t telnet_negotiation[] = {
        0xFF, 0xFB, 0x01, /* IAC WILL ECHO */
        0xFF, 0xFB, 0x03  /* IAC WILL SGA */
    };
    netconn_write(conn, telnet_negotiation, sizeof(telnet_negotiation), NETCONN_COPY);

    /* Send Welcome Message */
    telnet_write("\r\nWelcome to CLI Telnet Server\r\n> ");

    while ((err = netconn_recv(conn, &buf)) == ERR_OK)
    {
        do
        {
            netbuf_data(buf, &data, &len);
            uint8_t *cdata = (uint8_t *)data;

            for (int i = 0; i < len; i++)
            {
                uint8_t c = cdata[i];

                /* Handle/Skip Telnet IAC commands (3 bytes usually: IAC WILL/DO OPTION) */
                if (c == 0xFF)
                {
                    /* Simple skip: jump over the next 2 bytes if they exist in this buffer */
                    i += 2;
                    continue;
                }

                if (esc_state == 1)
                {
                    if (c == '[' || c == 'O')
                        esc_state = 2;
                    else
                        esc_state = 0;
                }
                else if (esc_state == 2)
                {
                    if (c == 'A') /* Up Arrow */
                    {
                        if (history_count > 0 && history_pos < history_count)
                        {
                            history_pos++;
                            int idx = (history_head - history_pos + HISTORY_DEPTH) % HISTORY_DEPTH;
                            /* Clear Line */
                            while (cmd_len > 0)
                            {
                                telnet_write("\b \b");
                                cmd_len--;
                            }

                            strncpy(cmd_buf, telnet_history[idx], MAX_CMD_LEN - 1);
                            cmd_buf[MAX_CMD_LEN - 1] = '\0';
                            cmd_len = strlen(cmd_buf);
                            telnet_write(cmd_buf);
                        }
                    }
                    else if (c == 'B') /* Down Arrow */
                    {
                        if (history_pos > 0)
                        {
                            history_pos--;
                            /* Clear Line */
                            while (cmd_len > 0)
                            {
                                telnet_write("\b \b");
                                cmd_len--;
                            }

                            if (history_pos > 0)
                            {
                                int idx =
                                    (history_head - history_pos + HISTORY_DEPTH) % HISTORY_DEPTH;
                                strncpy(cmd_buf, telnet_history[idx], MAX_CMD_LEN - 1);
                                cmd_buf[MAX_CMD_LEN - 1] = '\0';
                                cmd_len = strlen(cmd_buf);
                                telnet_write(cmd_buf);
                            }
                            else /* Empty */
                            {
                                cmd_buf[0] = '\0';
                                cmd_len = 0;
                            }
                        }
                    }
                    esc_state = 0;
                }
                else /* Normal State */
                {
                    if (c == 0x1B) /* ESC */
                    {
                        esc_state = 1;
                    }
                    else if (c == '\r' || c == '\n')
                    {
                        if (cmd_len > 0)
                        {
                            cmd_buf[cmd_len] = '\0';
                            telnet_write("\r\n");
                            telnet_add_history(cmd_buf);

                            shell_process(cmd_buf, telnet_write);

                            cmd_len = 0;
                            history_pos = 0;
                        }
                        else
                        {
                            telnet_write("\r\n");
                        }
                        telnet_write("\r\n> ");
                    }
                    else if (c == 0x08 || c == 0x7F) /* Backspace */
                    {
                        if (cmd_len > 0)
                        {
                            cmd_len--;
                            telnet_write("\b \b");
                        }
                    }
                    else if (c == 0x09) /* TAB */
                    {
                        int old_len = cmd_len;
                        if (shell_autocomplete(cmd_buf, &cmd_len, MAX_CMD_LEN))
                        {
                            /* Echo appended part */
                            if (cmd_len > old_len)
                            {
                                telnet_write(&cmd_buf[old_len]);
                            }
                        }
                    }
                    else if (c >= 32 && c <= 126)
                    {
                        if (cmd_len < MAX_CMD_LEN - 1)
                        {
                            cmd_buf[cmd_len++] = c;
                            char echo[2] = {c, 0};
                            telnet_write(echo);
                        }
                    }
                }
            }
        } while (netbuf_next(buf) >= 0);

        netbuf_delete(buf);
    }

    active_conn = NULL;
}

static void telnet_thread(void *arg)
{
    struct netconn *conn, *newconn;
    err_t err;
    LWIP_UNUSED_ARG(arg);

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, TELNET_PORT);
    netconn_listen(conn);
    uart_puts("[Telnet] Listening on port 23...\n");

    for (;;)
    {
        err = netconn_accept(conn, &newconn);
        if (err == ERR_OK)
        {
            uart_puts("[Telnet] Connection Accepted.\n");
            telnet_process_connection(newconn);
            netconn_delete(newconn);
            uart_puts("[Telnet] Connection Closed.\n");
        }
    }
}

void telnet_init(void)
{
    sys_thread_new("telnet_thread", telnet_thread, NULL, TELNET_THREAD_STACKSIZE,
                   TELNET_THREAD_PRIO);
}
