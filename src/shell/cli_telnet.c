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

static void telnet_process_connection(struct netconn *conn)
{
    struct netbuf *buf;
    void *data;
    u16_t len;
    err_t err;
    char line_buffer[128];
    int line_len = 0;

    active_conn = conn; // Simplistic approach: only supports one "active" output for the callback

    /* Send Welcome Message */
    telnet_write("\r\nWelcome to CLI Telnet Server\r\n> ");

    while ((err = netconn_recv(conn, &buf)) == ERR_OK)
    {
        do
        {
            netbuf_data(buf, &data, &len);
            char *cdata = (char *)data;

            for (int i = 0; i < len; i++)
            {
                char c = cdata[i];

                /* Basic Line Editing */
                if (c == '\r' || c == '\n')
                {
                    if (line_len > 0)
                    {
                        line_buffer[line_len] = '\0';
                        telnet_write("\r\n"); // Echo newline

                        /* Execute Command */
                        shell_process(line_buffer, telnet_write);

                        line_len = 0;
                        telnet_write("\r\n> "); // Prompt
                    }
                }
                else if (c == 0x7F || c == 0x08) /* Backspace */
                {
                    if (line_len > 0)
                    {
                        line_len--;
                        telnet_write("\b \b");
                    }
                }
                else if (line_len < sizeof(line_buffer) - 1)
                {
                    line_buffer[line_len++] = c;
                    /* Local Echo */
                    char tmp[2] = {c, 0};
                    telnet_write(tmp);
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
