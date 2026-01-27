#include "Filter.h"

bool parse_mac(const char *str, unsigned char *out)
{
    int values[6];
    if (sscanf(str, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4],
               &values[5]) == 6)
    {
        for (int i = 0; i < 6; i++)
            out[i] = (unsigned char)values[i];
        return true;
    }
    return false;
}

bool parse_hex(const char *str, unsigned short *out)
{
    int val;
    if (sscanf(str, "0x%x", &val) == 1 || sscanf(str, "%x", &val) == 1)
    {
        *out = (unsigned short)val;
        return true;
    }
    return false;
}

bool packet_matches_filter(const unsigned char *packet, int len, const Filter &f)
{
    if (!f.enabled)
        return true;
    if (len < 14)
        return false;

    struct eth_header *eh = (struct eth_header *)packet;

    if (f.has_src && memcmp(eh->h_source, f.src, 6) != 0)
        return false;
    if (f.has_dst && memcmp(eh->h_dest, f.dst, 6) != 0)
        return false;
    if (f.has_type && ntohs(eh->h_proto) != f.type)
        return false;

    return true;
}
