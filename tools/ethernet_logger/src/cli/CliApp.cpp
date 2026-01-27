#include "CliApp.h"
#include "../core/Capture.h"
#include <cstdio>
#include <ctime>

namespace CLI
{

int run(bool l2_mode, const std::string &iface, const Filter &filter)
{
    if (l2_mode)
    {
        L2Capture l2;
        if (!l2.open_capture(iface))
        {
            fprintf(stderr, "Failed to open interface: %s\n", iface.c_str());
            return 1;
        }

        printf("Ethernet Logger (CLI L2 Mode) on %s\n", iface.c_str());
        printf("Press Ctrl+C to exit.\n");

        std::vector<LogEntry> logs;
        // We don't really use logs vector for CLI as we print directly in process_packet
        // But L2Capture::poll expects it.

        while (true)
        {
            l2.poll(logs, filter);
            logs.clear(); // Clear to prevent accumulation
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
                if (packet_matches_filter(buffer, n, filter))
                {
                    struct eth_header *eh = (struct eth_header *)buffer;
                    unsigned short ether_type = ntohs(eh->h_proto);

                    if (ether_type == ETH_P_LOG)
                    {
                        std::string msg((char *)(buffer + 14), n - 14);
                        if (!msg.empty() && msg.back() == '\0')
                            msg.pop_back();

                        double current_time = ((double)clock() / CLOCKS_PER_SEC) - start_time;
                        printf("[%.3f] (%d bytes) %s\n", current_time, n, msg.c_str());
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
