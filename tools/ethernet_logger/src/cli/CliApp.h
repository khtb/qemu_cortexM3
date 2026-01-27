#pragma once
#include "../core/Filter.h"
#include <string>

namespace CLI
{
int run(bool l2_mode, const std::string &iface, const Filter &filter);
}
