#ifndef LWIP_LWIPOPTS_H
#define LWIP_LWIPOPTS_H

/* Threading settings */
#define NO_SYS 0
#define LWIP_SOCKET 1
#define LWIP_NETCONN 1

/* Memory settings */
#define MEM_ALIGNMENT 4
#define MEM_SIZE (4 * 1024)
#define MEMP_NUM_PBUF 16
#define MEMP_NUM_UDP_PCB 4
#define MEMP_NUM_TCP_PCB 4
#define MEMP_NUM_TCP_PCB_LISTEN 4
#define MEMP_NUM_TCP_SEG 16
#define MEMP_NUM_SYS_TIMEOUT 8

/* Pbuf settings */
#define PBUF_POOL_SIZE 8
#define PBUF_POOL_BUFSIZE 512

/* TCP settings */
#define LWIP_TCP 1
#define LWIP_SO_RCVTIMEO 1
#define TCP_TTL 255
#define TCP_WND (2 * TCP_MSS)
#define TCP_MAXRTX 12
#define TCP_SYNMAXRTX 6
#define TCP_MSS 1460

/* UDP settings */
#define LWIP_UDP 1
#define UDP_TTL 255

/* ICMP settings */
#define LWIP_ICMP 1
#define LWIP_RAW 1

/* DNS settings */
#define LWIP_DNS 0

/* Checksum settings */
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1

/* Debugging */
#define LWIP_DEBUG 1
#define IP_DEBUG LWIP_DBG_ON
#define TCP_DEBUG LWIP_DBG_ON
#define UDP_DEBUG LWIP_DBG_ON
#define ICMP_DEBUG LWIP_DBG_ON
#define ETHARP_DEBUG LWIP_DBG_ON
#define NETIF_DEBUG LWIP_DBG_ON
#define SYS_DEBUG LWIP_DBG_ON
#define API_LIB_DEBUG LWIP_DBG_ON
#define API_MSG_DEBUG LWIP_DBG_ON

/* OS components */
#define TCPIP_THREAD_NAME "tcpip_thread"
#define TCPIP_THREAD_STACKSIZE 1024
#define TCPIP_THREAD_PRIO 3
#define TCPIP_MBOX_SIZE 16
#define DEFAULT_RAW_RECVMBOX_SIZE 16
#define DEFAULT_UDP_RECVMBOX_SIZE 16
#define DEFAULT_TCP_RECVMBOX_SIZE 16
#define DEFAULT_ACCEPTMBOX_SIZE 16
#define DEFAULT_THREAD_STACKSIZE 512
#define DEFAULT_THREAD_PRIO 1

/* Stats */
#define LWIP_STATS 0

#endif /* LWIP_LWIPOPTS_H */
