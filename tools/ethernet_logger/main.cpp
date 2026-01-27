#include "src/cli/CliApp.h"
#include "src/core/AppConfig.h"
#include "src/core/Device.h"
#include "src/core/Platform.h"
#include "src/gui/GuiApp.h"
#include <memory>

int main(int argc, char **argv)
{
    Core::AppConfig config;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--cli") == 0)
        {
            config.cli_mode = true;
        }
        else if (strcmp(argv[i], "--list") == 0)
        {
            list_devices();
            return 0;
        }
        else if (strcmp(argv[i], "--l2") == 0 && i + 1 < argc)
        {
            config.l2_mode = true;
            config.iface = argv[++i];
            config.filter.enabled = true;
        }
        else if (strcmp(argv[i], "--src") == 0 && i + 1 < argc)
        {
            if (parse_mac(argv[++i], config.filter.src))
                config.filter.has_src = true;
            else
                printf("Invalid Source MAC: %s\n", argv[i]);
        }
        else if (strcmp(argv[i], "--dst") == 0 && i + 1 < argc)
        {
            if (parse_mac(argv[++i], config.filter.dst))
                config.filter.has_dst = true;
            else
                printf("Invalid Dest MAC: %s\n", argv[i]);
        }
        else if (strcmp(argv[i], "--type") == 0 && i + 1 < argc)
        {
            if (parse_hex(argv[++i], &config.filter.type))
                config.filter.has_type = true;
            else
                printf("Invalid EtherType: %s\n", argv[i]);
        }
        else if (strcmp(argv[i], "--payload") == 0)
        {
            config.payload_only = true;
        }
    }

    std::unique_ptr<Core::IApp> app;

    if (config.cli_mode)
    {
        app = std::unique_ptr<Core::IApp>(new CLI::CliApp(config));
    }
    else
    {
        app = std::unique_ptr<Core::IApp>(new GUI::GuiApp(config));
    }

    return app->run();
}
