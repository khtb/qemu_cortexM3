#pragma once
#include "Constants.h"
#include "Platform.h"

struct Filter
{
    bool enabled;
    bool has_src;
    unsigned char src[6];
    bool has_dst;
    unsigned char dst[6];
    bool has_vlan;
    unsigned short vlan_id;
    bool has_type;
    unsigned short type;

    Filter() : enabled(false), has_src(false), has_dst(false), has_vlan(false), vlan_id(0), has_type(false), type(0) {}
};

bool parse_mac(const char *str, unsigned char *out);
bool parse_hex(const char *str, unsigned short *out);
bool packet_matches_filter(const unsigned char *packet, int len, const Filter &f);
