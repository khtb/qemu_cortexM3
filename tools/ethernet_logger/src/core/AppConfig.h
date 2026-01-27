#pragma once
#include "Filter.h"
#include <string>

namespace Core
{
struct AppConfig
{
    bool cli_mode = false;
    bool l2_mode = false;
    std::string iface;
    Filter filter;
    bool payload_only = false;
};
} // namespace Core
