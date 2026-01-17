#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/api.h"
#include "lwip/sys.h"
#include <string.h>

static void echo_thread(void *arg)
{
    struct netconn *conn, *newconn;
    err_t err;
    LWIP_UNUSED_ARG(arg);

    /* Create a new connection identifier. */
    conn = netconn_new(NETCONN_TCP);

    if (conn != NULL)
    {
        /* Bind connection to well known port 7. */
        err = netconn_bind(conn, IP_ADDR_ANY, 7);

        if (err == ERR_OK)
        {
            uart_puts("[Echo] Bound to port 7. Listening...\n");
            /* Tell connection to go into listening mode. */
            netconn_listen(conn);

            while (1)
            {
                /* Grab new connection. */
                err = netconn_accept(conn, &newconn);

                /* Process the new connection. */
                if (err == ERR_OK)
                {
                    uart_puts("[Echo] Accepted new connection!\n");

                    /* Set a 1-second timeout for receive */
                    netconn_set_recvtimeout(newconn, 1000);

                    struct netbuf *buf;
                    void *data;
                    u16_t len;

                    while (1)
                    {
                        err = netconn_recv(newconn, &buf);

                        if (err == ERR_OK)
                        {
                            do
                            {
                                netbuf_data(buf, &data, &len);
                                netconn_write(newconn, data, len, NETCONN_COPY);
                            } while (netbuf_next(buf) >= 0);
                            netbuf_delete(buf);
                        }
                        else if (err == ERR_TIMEOUT)
                        {
                            /* Send periodic Hello World */
                            const char *msg = "Hello World\n";
                            netconn_write(newconn, msg, strlen(msg), NETCONN_COPY);
                        }
                        else
                        {
                            /* Connection closed or other error */
                            break;
                        }
                    }

                    /* Close connection and discard connection identifier. */
                    netconn_close(newconn);
                    netconn_delete(newconn);
                }
            }
        }
        else
        {
            netconn_delete(conn);
        }
    }
}

void echo_init(void)
{
    uart_puts("[Echo] Initializing TCP Echo Server on port 7...\n");
    sys_thread_new("echo_thread", echo_thread, NULL, 512, 1);
}

#endif /* LWIP_NETCONN */
