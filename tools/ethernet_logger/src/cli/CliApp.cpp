#include "CliApp.h"
#include "../core/Capture.h"
#include <cstdio>
#include <ctime>

namespace CLI
{

CliApp::CliApp(const Core::AppConfig &config) : m_config(config) {}

int CliApp::run()
{
    if (m_config.l2_mode)
    {
        L2Capture l2;
        if (!l2.open_capture(m_config.iface))
        {
            fprintf(stderr, "Failed to open interface: %s\n", m_config.iface.c_str());
            return 1;
        }

        if (!m_config.payload_only)
        {
            printf("Ethernet Logger (CLI L2 Mode) on %s\n", m_config.iface.c_str());
            printf("Press Ctrl+C to exit.\n");
        }

        std::vector<LogEntry> logs;
        while (true)
        {
            l2.poll(logs, m_config.filter);
            if (m_config.payload_only)
            {
                for (const auto &log : logs)
                {
                    printf("%s\n", log.message.c_str());
                }
                fflush(stdout);
            }
            logs.clear();
            SLEEP_MS(1);
        }
    }
    else
    {
        int sockfd = open_udp_socket(12345);
        if (sockfd < 0)
        {
            fprintf(stderr, "Failed to open UDP socket\n");
            return 1;
        }

        if (!m_config.payload_only)
            printf("Ethernet Logger (CLI UDP Mode)\nListening on :12345\n");

        unsigned char buffer[2048];
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);
        double start_time = (double)clock() / CLOCKS_PER_SEC;

        while (true)
        {
            int n = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr,
                             &len);
            if (n < 0)
            {
                SLEEP_MS(1);
                continue;
            }

            if (n >= 14)
            {
                if (packet_matches_filter(buffer, n, m_config.filter))
                {
                    struct eth_header *eh = (struct eth_header *)buffer;
                    unsigned short ether_type = ntohs(eh->h_proto);

                    if (m_config.filter.has_type ? (ether_type == m_config.filter.type)
                                                 : (ether_type == ETH_P_LOG))
                    {
                        std::string msg((char *)(buffer + 14), n - 14);
                        if (!msg.empty() && msg.back() == '\0')
                            msg.pop_back();

                        if (m_config.payload_only)
                        {
                            printf("%s\n", msg.c_str());
                        }
                        else
                        {
                            double current_time = ((double)clock() / CLOCKS_PER_SEC) - start_time;
                            printf("[%.3f] (%d bytes) %s\n", current_time, n, msg.c_str());
                        }
                        fflush(stdout);
                    }
                }
            }
        }
        CLOSE_SOCKET(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
    }
    return 0;
}

} // namespace CLI
