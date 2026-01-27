#include "src/cli/CliApp.h"
#include "src/core/Device.h"
#include "src/core/Filter.h"
#include "src/core/Platform.h"
#include "src/gui/GuiApp.h"

int main(int argc, char **argv)
{
    bool cli_mode = false;
    bool l2_mode = false;
    std::string l2_iface;
    Filter filter;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--cli") == 0)
        {
            cli_mode = true;
        }
        else if (strcmp(argv[i], "--list") == 0)
        {
            // Just assume Device.h is working fine
            list_devices();
            return 0;
        }
        else if (strcmp(argv[i], "--l2") == 0 && i + 1 < argc)
        {
            l2_mode = true;
            l2_iface = argv[++i];
            filter.enabled = true;
        }
        else if (strcmp(argv[i], "--src") == 0 && i + 1 < argc)
        {
            if (parse_mac(argv[++i], filter.src))
                filter.has_src = true;
            else
                printf("Invalid Source MAC: %s\n", argv[i]);
        }
        else if (strcmp(argv[i], "--dst") == 0 && i + 1 < argc)
        {
            if (parse_mac(argv[++i], filter.dst))
                filter.has_dst = true;
            else
                printf("Invalid Dest MAC: %s\n", argv[i]);
        }
        else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc)
        {
            if (parse_hex(argv[++i], &filter.type))
                filter.has_type = true;
            else
                printf("Invalid EtherType: %s\n", argv[i]);
        }
    }

    if (cli_mode)
    {
        return CLI::run(l2_mode, l2_iface, filter);
    }
    else
    {
        return GUI::run(l2_mode, l2_iface, filter);
    }
}
