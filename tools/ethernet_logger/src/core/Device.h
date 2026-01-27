#pragma once
#include "Platform.h"

struct DeviceInfo
{
    std::string name;
    std::string description;
    std::vector<std::string> addresses;
    std::string display_name;
};

std::vector<DeviceInfo> get_device_list();
void list_devices(); // Print to stdout
