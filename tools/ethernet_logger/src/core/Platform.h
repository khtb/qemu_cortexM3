#pragma once

#ifdef _WIN32
#include <pcap.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "packet.lib")

#define CLOSE_SOCKET(s) closesocket(s)
#define IS_VALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define SOCKET_AGAIN (WSAGetLastError() == WSAEWOULDBLOCK)
#define SLEEP_MS(x) Sleep(x)
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <net/bpf.h>
#include <sys/time.h>
#include <sys/types.h>
#elif defined(__linux__)
#include <linux/if_packet.h>
#include <net/ethernet.h>
#endif

#define CLOSE_SOCKET(s) close(s)
#define IS_VALIDSOCKET(s) ((s) >= 0)
#define SOCKET_AGAIN (errno == EAGAIN || errno == EWOULDBLOCK)
#define SLEEP_MS(x) usleep((x) * 1000)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

#include <set>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
