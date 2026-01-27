#include "Capture.h"
#include <ctime>

// Simple Time Helper (Generic)
static double GetTimeSeconds() { return (double)clock() / CLOCKS_PER_SEC; }

L2Capture::~L2Capture() { close_capture(); }

bool L2Capture::open_capture(const std::string &iface)
{
    close_capture();
    this->interface_name = iface;

#ifdef _WIN32
    char errbuf[PCAP_ERRBUF_SIZE];
    if ((adhandle = pcap_open_live(iface.c_str(), 65536, 1, 1, errbuf)) == NULL)
    {
        fprintf(stderr, "Error opening adapter: %s\n", errbuf);
        return false;
    }
#elif defined(__APPLE__)
    for (int i = 0; i < 99; i++)
    {
        char buf[32];
        sprintf(buf, "/dev/bpf%d", i);
        bpf_fd = open(buf, O_RDWR);
        if (bpf_fd >= 0)
            break;
    }
    if (bpf_fd < 0)
        return false;

    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));
    if (ioctl(bpf_fd, BIOCSETIF, &ifr) < 0)
    {
        close(bpf_fd);
        return false;
    }

    unsigned int enable = 1;
    ioctl(bpf_fd, BIOCIMMEDIATE, &enable);

    int flags = fcntl(bpf_fd, F_GETFL, 0);
    fcntl(bpf_fd, F_SETFL, flags | O_NONBLOCK);

    if (ioctl(bpf_fd, BIOCGBLEN, &bpf_buf_len) < 0)
        bpf_buf_len = 4096;
    bpf_buf = new unsigned char[bpf_buf_len];
#elif defined(__linux__)
    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock_fd < 0)
        return false;

    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));
    if (ioctl(sock_fd, SIOCGIFINDEX, &ifr) < 0)
    {
        close(sock_fd);
        return false;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);
    if (bind(sock_fd, (struct sockaddr *)&sll, sizeof(sll)) < 0)
    {
        close(sock_fd);
        return false;
    }

    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(sock_fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0)
    {
        perror("setsockopt(PACKET_MR_PROMISC) failed (ignoring)");
    }

    int flags = fcntl(sock_fd, F_GETFL, 0);
    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
#endif
    return true;
}

void L2Capture::close_capture()
{
#ifdef _WIN32
    if (adhandle)
    {
        pcap_close(adhandle);
        adhandle = nullptr;
    }
#elif defined(__APPLE__)
    if (bpf_fd >= 0)
    {
        close(bpf_fd);
        bpf_fd = -1;
    }
    if (bpf_buf)
    {
        delete[] bpf_buf;
        bpf_buf = nullptr;
    }
#elif defined(__linux__)
    if (sock_fd >= 0)
    {
        close(sock_fd);
        sock_fd = -1;
    }
#endif
}

int L2Capture::poll(std::vector<LogEntry> &logs, const Filter &filter)
{
    int count = 0;
#ifdef _WIN32
    if (!adhandle)
        return 0;
    struct pcap_pkthdr *header;
    const unsigned char *pkt_data;
    int res;
    for (int i = 0; i < 10; i++)
    {
        res = pcap_next_ex(adhandle, &header, &pkt_data);
        if (res > 0)
        {
            process_packet(pkt_data, header->caplen, logs, filter);
            count++;
        }
        else
        {
            break;
        }
    }
#elif defined(__APPLE__)
    if (bpf_fd < 0)
        return 0;
    ssize_t n = read(bpf_fd, bpf_buf, bpf_buf_len);
    if (n > 0)
    {
        unsigned char *p = bpf_buf;
        unsigned char *end = bpf_buf + n;
        while (p < end)
        {
            struct bpf_hdr *bh = (struct bpf_hdr *)p;
            process_packet(p + bh->bh_hdrlen, bh->bh_caplen, logs, filter);
            count++;
            p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
        }
    }
#elif defined(__linux__)
    if (sock_fd < 0)
        return 0;
    unsigned char buffer[65535];
    while (true)
    {
        int n = recvfrom(sock_fd, buffer, sizeof(buffer), 0, NULL, NULL);
        if (n <= 0)
            break;
        process_packet(buffer, n, logs, filter);
        count++;
        if (count > 10)
            break;
    }
#endif
    return count;
}

void L2Capture::process_packet(const unsigned char *pkt, int len, std::vector<LogEntry> &logs,
                               const Filter &filter)
{
    if (packet_matches_filter(pkt, len, filter))
    {
        struct eth_header *eh = (struct eth_header *)pkt;
        unsigned short ether_type = ntohs(eh->h_proto);

        // Check if this is a "Log" packet either by active filter or default constant
        bool is_log_type =
            filter.has_type ? (ether_type == filter.type) : (ether_type == ETH_P_LOG);

        if (is_log_type && len > 14)
        {
            std::string msg((char *)(pkt + 14), len - 14);
            // Trim nulls if present at end
            if (!msg.empty() && msg.back() == '\0')
                msg.pop_back();

            logs.push_back({GetTimeSeconds(), msg, len});
            // Print to console potentially (CLI mode relies on this or polls logs)
            // But CliApp::run handles printing too?
            // Actually CliApp L2 mode prints logs from 'logs' vector.
            // But L2Capture::process_packet ALSO prints to stdout?
            // Line 189: printf("[%.3f] ...")
            // We should align behavior.
            // If we are in GUI mode, L2Capture printing to stdout is maybe annoying or useful
            // debug. Let's keep it but formatted.
            printf("[%.3f] (%d bytes) %s\n", GetTimeSeconds(), len, msg.c_str());
        }
        else
        {
            char msg_buf[256];
            snprintf(
                msg_buf, sizeof(msg_buf),
                "Src:%02X:%02X:%02X:%02X:%02X:%02X Dst:%02X:%02X:%02X:%02X:%02X:%02X Type:0x%04X",
                eh->h_source[0], eh->h_source[1], eh->h_source[2], eh->h_source[3], eh->h_source[4],
                eh->h_source[5], eh->h_dest[0], eh->h_dest[1], eh->h_dest[2], eh->h_dest[3],
                eh->h_dest[4], eh->h_dest[5], ether_type);

            logs.push_back({GetTimeSeconds(), std::string(msg_buf), len});
            printf("[%.3f] (%d bytes) %s\n", GetTimeSeconds(), len, msg_buf);
        }
        fflush(stdout);
    }
}

int open_udp_socket(int port)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return -1;
#endif

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (!IS_VALIDSOCKET(sockfd))
        return -1;

#ifdef _WIN32
    unsigned long mode = 1;
    if (ioctlsocket(sockfd, FIONBIO, &mode) != 0)
    {
        CLOSE_SOCKET(sockfd);
        return -1;
    }
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        CLOSE_SOCKET(sockfd);
        return -1;
    }
    return sockfd;
}
