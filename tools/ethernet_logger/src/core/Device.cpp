#include "Device.h"

std::vector<DeviceInfo> get_device_list()
{
    std::vector<DeviceInfo> devices;
#ifdef _WIN32
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];
    if (pcap_findalldevs(&alldevs, errbuf) != -1 && alldevs != NULL)
    {
        for (pcap_if_t *d = alldevs; d; d = d->next)
        {
            DeviceInfo dev;
            dev.name = d->name ? d->name : "";
            dev.description = d->description ? d->description : "(No description)";

            for (pcap_addr_t *a = d->addresses; a; a = a->next)
            {
                if (a->addr && a->addr->sa_family == AF_INET)
                {
                    struct sockaddr_in *sin = (struct sockaddr_in *)a->addr;
                    dev.addresses.push_back(inet_ntoa(sin->sin_addr));
                }
                else if (a->addr && a->addr->sa_family == AF_INET6)
                {
                    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)a->addr;
                    char ip6str[128];
                    if (getnameinfo((struct sockaddr *)sin6, sizeof(struct sockaddr_in6), ip6str,
                                    sizeof(ip6str), NULL, 0, NI_NUMERICHOST) == 0)
                        dev.addresses.push_back(ip6str);
                }
            }
            dev.display_name = dev.description;
            if (!dev.addresses.empty())
            {
                dev.display_name += " (" + dev.addresses[0] + ")";
            }
            else
            {
                dev.display_name += " (" + dev.name + ")";
            }
            devices.push_back(dev);
        }
        pcap_freealldevs(alldevs);
    }
#else
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != -1)
    {
        std::set<std::string> seen;
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == NULL)
                continue;
            std::string name = ifa->ifa_name;
            if (seen.find(name) == seen.end())
            {
                seen.insert(name);
                DeviceInfo dev;
                dev.name = name;
                dev.description = name;
                dev.display_name = name;
                devices.push_back(dev);
            }
        }
        freeifaddrs(ifaddr);
    }
#endif
    return devices;
}

void list_devices()
{
    auto devices = get_device_list();
    printf("Available Network Devices:\n==========================\n");
    for (const auto &dev : devices)
    {
        printf("Name: %s\n      Description: %s\n", dev.name.c_str(), dev.description.c_str());
        for (const auto &addr : dev.addresses)
        {
            printf("      Address: %s\n", addr.c_str());
        }
        printf("\n");
    }
}
