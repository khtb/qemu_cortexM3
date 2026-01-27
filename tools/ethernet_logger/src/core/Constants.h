#pragma once
#include <string>

#define ETH_ALEN 6
#define ETH_P_LOG 0x88B5

struct eth_header
{
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    unsigned short h_proto;
};

struct LogEntry
{
    double timestamp;
    std::string message;
    int length;
};
