// Headers
#ifdef _WIN32
#include <pcap.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wpcap.lib")
#pragma comment(lib, "packet.lib")
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <net/bpf.h>
#include <sys/time.h>
#include <sys/types.h>
#elif defined(__linux__)
#include <linux/if_packet.h>
#include <net/ethernet.h>
#endif
#endif

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <ctime>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define CLOSE_SOCKET(s) closesocket(s)
#define IS_VALIDSOCKET(s) ((s) != INVALID_SOCKET)
#define SOCKET_AGAIN (WSAGetLastError() == WSAEWOULDBLOCK)
#define SLEEP_MS(x) Sleep(x)
typedef int socklen_t;
#else
#include <ifaddrs.h>
#define CLOSE_SOCKET(s) close(s)
#define IS_VALIDSOCKET(s) ((s) >= 0)
#define SOCKET_AGAIN (errno == EAGAIN || errno == EWOULDBLOCK)
#define SLEEP_MS(x) usleep((x) * 1000)
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

// Ethernet Constants
#define ETH_ALEN 6
#define ETH_P_LOG 0x88B5

struct eth_header
{
    unsigned char h_dest[ETH_ALEN];
    unsigned char h_source[ETH_ALEN];
    unsigned short h_proto;
};

struct LogEntry
{
    double timestamp;
    std::string message;
    int length;
};

struct Filter
{
    bool enabled;
    bool has_src;
    unsigned char src[6];
    bool has_dst;
    unsigned char dst[6];
    bool has_type;
    unsigned short type;

    Filter() : enabled(false), has_src(false), has_dst(false), has_type(false), type(0) {}
};

bool parse_mac(const char *str, unsigned char *out)
{
    int values[6];
    if (sscanf(str, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2], &values[3], &values[4],
               &values[5]) == 6)
    {
        for (int i = 0; i < 6; i++)
            out[i] = (unsigned char)values[i];
        return true;
    }
    return false;
}

bool parse_hex(const char *str, unsigned short *out)
{
    int val;
    if (sscanf(str, "0x%x", &val) == 1 || sscanf(str, "%x", &val) == 1)
    {
        *out = (unsigned short)val;
        return true;
    }
    return false;
}

bool packet_matches_filter(const unsigned char *packet, int len, const Filter &f)
{
    if (!f.enabled)
        return true;
    if (len < 14)
        return false;

    struct eth_header *eh = (struct eth_header *)packet;

    if (f.has_src && memcmp(eh->h_source, f.src, 6) != 0)
        return false;
    if (f.has_dst && memcmp(eh->h_dest, f.dst, 6) != 0)
        return false;
    if (f.has_type && ntohs(eh->h_proto) != f.type)
        return false;

    return true;
}

void print_hex_dump(const unsigned char *data, int len)
{
    for (int i = 0; i < len; ++i)
    {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

void list_devices()
{
    printf("d Available Network Devices:\n");
    printf("==========================\n");

#ifdef _WIN32
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == -1)
    {
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
        return;
    }

    if (alldevs == NULL)
    {
        printf("No devices found! Make sure Npcap is installed.\n");
        return;
    }

    for (pcap_if_t *d = alldevs; d; d = d->next)
    {
        printf("Name: %s\n", d->name);
        if (d->description)
            printf("      Description: %s\n", d->description);
        else
            printf("      Description: (No description available)\n");
        printf("\n");
    }

    pcap_freealldevs(alldevs);

#else
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return;
    }

    std::set<std::string> devices;

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        // Only list AF_PACKET (Linux) or AF_LINK (BSD/macOS) to see physical/logical interfaces
        // But to be generic, we can just list names.
        devices.insert(ifa->ifa_name);
    }

    for (const auto &dev : devices)
    {
        printf(" - %s\n", dev.c_str());
    }

    freeifaddrs(ifaddr);
#endif
    printf("\n");
}

void run_l2_mode(const std::string &iface, const Filter &filter)
{
    printf("Ethernet Logger (L2 Raw Mode) on Interface: %s\n", iface.c_str());
    if (filter.has_src)
    {
        printf("Filter Src: %02X:%02X:%02X:%02X:%02X:%02X\n", filter.src[0], filter.src[1],
               filter.src[2], filter.src[3], filter.src[4], filter.src[5]);
    }
    if (filter.has_dst)
    {
        printf("Filter Dst: %02X:%02X:%02X:%02X:%02X:%02X\n", filter.dst[0], filter.dst[1],
               filter.dst[2], filter.dst[3], filter.dst[4], filter.dst[5]);
    }
    if (filter.has_type)
    {
        printf("Filter Type: 0x%04X\n", filter.type);
    }
    printf("Press Ctrl+C to exit.\n\n");

#if defined(_WIN32)
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *adhandle;

    // Open the adapter
    // Note: The interface name on Windows needs to be the device name (e.g., \Device\NPF_{...})
    // or the simple name if Npcap is configured to support it, but usually it complicates things.
    // For now, we assume the user passes the correct device name string.

    // We use pcap_open_live
    // snaplen: 65536, promisc: 1, to_ms: 1000
    if ((adhandle = pcap_open_live(iface.c_str(), 65536, 1, 1000, errbuf)) == NULL)
    {
        fprintf(stderr, "\nUnable to open the adapter. %s is not supported by Npcap/WinPcap\n",
                iface.c_str());
        fprintf(stderr, "Error: %s\n", errbuf);
        return;
    }

    printf("Listening on %s...\n", iface.c_str());

    // We don't filter at the pcap kernel level here to keep logic consistent with shared code,
    // but pcap_compile/pcap_setfilter could be used for performance.
    // We will use the user-space filter we already have.

    struct pcap_pkthdr *header;
    const unsigned char *pkt_data;
    int res;

    double start_time = (double)clock() / CLOCKS_PER_SEC;

    while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0)
    {
        if (res == 0)
            continue; // Timeout

        if (packet_matches_filter(pkt_data, header->caplen, filter))
        {
            double current_time = ((double)clock() / CLOCKS_PER_SEC) - start_time;
            struct eth_header *eh = (struct eth_header *)pkt_data;
            printf("[%.3f] (%d bytes) %02X:%02X:%02X:%02X:%02X:%02X -> "
                   "%02X:%02X:%02X:%02X:%02X:%02X Type:0x%04X\n",
                   current_time, header->caplen, eh->h_source[0], eh->h_source[1], eh->h_source[2],
                   eh->h_source[3], eh->h_source[4], eh->h_source[5], eh->h_dest[0], eh->h_dest[1],
                   eh->h_dest[2], eh->h_dest[3], eh->h_dest[4], eh->h_dest[5], ntohs(eh->h_proto));
            fflush(stdout);
        }
    }

    if (res == -1)
    {
        printf("Error reading the packets: %s\n", pcap_geterr(adhandle));
    }

    pcap_close(adhandle);
    return;
#elif defined(__APPLE__)
    // macOS BPF Implementation
    int bpf = -1;
    for (int i = 0; i < 99; i++)
    {
        char buf[32];
        sprintf(buf, "/dev/bpf%d", i);
        bpf = open(buf, O_RDWR);
        if (bpf >= 0)
        {
            printf("Opened %s\n", buf);
            break;
        }
    }
    if (bpf < 0)
    {
        perror("No BPF device available");
        return;
    }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));
    if (ioctl(bpf, BIOCSETIF, &ifr) < 0)
    {
        perror("BIOCSETIF");
        close(bpf);
        return;
    }

    unsigned int enable = 1;
    if (ioctl(bpf, BIOCIMMEDIATE, &enable) < 0)
    {
        perror("BIOCIMMEDIATE");
    }

    // Use a reasonable buffer size for BPF
    int buf_len = 0;
    if (ioctl(bpf, BIOCGBLEN, &buf_len) < 0)
    {
        buf_len = 4096; // Fallback
    }

    unsigned char *buffer = new unsigned char[buf_len];
    double start_time = (double)clock() / CLOCKS_PER_SEC;

    while (true)
    {
        ssize_t n = read(bpf, buffer, buf_len);
        if (n <= 0)
        {
            if (n < 0 && errno != EAGAIN)
                perror("read");
            usleep(1000);
            continue;
        }

        unsigned char *p = buffer;
        unsigned char *end = buffer + n;

        while (p < end)
        {
            struct bpf_hdr *bh = (struct bpf_hdr *)p;
            unsigned char *pkt_data = p + bh->bh_hdrlen;

            if (packet_matches_filter(pkt_data, bh->bh_caplen, filter))
            {
                double current_time = ((double)clock() / CLOCKS_PER_SEC) - start_time;
                struct eth_header *eh = (struct eth_header *)pkt_data;
                printf("[%.3f] (%d bytes) %02X:%02X:%02X:%02X:%02X:%02X -> "
                       "%02X:%02X:%02X:%02X:%02X:%02X Type:0x%04X\n",
                       current_time, bh->bh_caplen, eh->h_source[0], eh->h_source[1],
                       eh->h_source[2], eh->h_source[3], eh->h_source[4], eh->h_source[5],
                       eh->h_dest[0], eh->h_dest[1], eh->h_dest[2], eh->h_dest[3], eh->h_dest[4],
                       eh->h_dest[5], ntohs(eh->h_proto));
                fflush(stdout);
            }

            p += BPF_WORDALIGN(bh->bh_hdrlen + bh->bh_caplen);
        }
    }
    delete[] buffer;
    close(bpf);

#elif defined(__linux__)
    // Linux AF_PACKET Implementation
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0)
    {
        perror("socket(AF_PACKET)");
        return;
    }

    struct ifreq ifr;
    strncpy(ifr.ifr_name, iface.c_str(), sizeof(ifr.ifr_name));
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0)
    {
        perror("ioctl(SIOCGIFINDEX)");
        close(sock);
        return;
    }

    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    if (bind(sock, (struct sockaddr *)&sll, sizeof(sll)) < 0)
    {
        perror("bind");
        close(sock);
        return;
    }

    // Set Promiscuous mode
    struct packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifr.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(sock, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0)
    {
        perror("setsockopt(PACKET_MR_PROMISC) failed (ignoring)");
    }

    unsigned char buffer[65535];
    double start_time = (double)clock() / CLOCKS_PER_SEC;

    while (true)
    {
        int n = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
        if (n < 0)
        {
            if (errno != EAGAIN)
                perror("recvfrom");
            usleep(1000);
            continue;
        }

        if (packet_matches_filter(buffer, n, filter))
        {
            double current_time = ((double)clock() / CLOCKS_PER_SEC) - start_time;
            struct eth_header *eh = (struct eth_header *)buffer;
            printf("[%.3f] (%d bytes) %02X:%02X:%02X:%02X:%02X:%02X -> "
                   "%02X:%02X:%02X:%02X:%02X:%02X Type:0x%04X\n",
                   current_time, n, eh->h_source[0], eh->h_source[1], eh->h_source[2],
                   eh->h_source[3], eh->h_source[4], eh->h_source[5], eh->h_dest[0], eh->h_dest[1],
                   eh->h_dest[2], eh->h_dest[3], eh->h_dest[4], eh->h_dest[5], ntohs(eh->h_proto));
            fflush(stdout);
        }
    }
    close(sock);
#endif
}

void run_cli_mode(int sockfd, const Filter &filter)
{
    printf("Ethernet Logger (CLI Mode)\n");
    if (filter.has_src)
    {
        printf("Filter Src: %02X:%02X:%02X:%02X:%02X:%02X\n", filter.src[0], filter.src[1],
               filter.src[2], filter.src[3], filter.src[4], filter.src[5]);
    }
    if (filter.has_dst)
    {
        printf("Filter Dst: %02X:%02X:%02X:%02X:%02X:%02X\n", filter.dst[0], filter.dst[1],
               filter.dst[2], filter.dst[3], filter.dst[4], filter.dst[5]);
    }
    if (filter.has_type)
    {
        printf("Filter Type: 0x%04X\n", filter.type);
    }
    printf("Listening on UDP :12345 for EtherType 0x%X\n", ETH_P_LOG);
    printf("Press Ctrl+C to exit.\n\n");

    unsigned char buffer[2048];
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);

    // Initial timestamp reference
    double start_time = (double)clock() / CLOCKS_PER_SEC;

    while (true)
    {
        int n =
            recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0)
        {
            if (!SOCKET_AGAIN)
            {
#ifdef _WIN32
                fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
#else
                perror("recvfrom");
#endif
                SLEEP_MS(10); // 10ms wait on error
            }
            else
            {
                SLEEP_MS(1); // 1ms wait
            }
            continue;
        }

        if (n >= 14)
        {
            if (!packet_matches_filter(buffer, n, filter))
                continue;

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
            list_devices();
            return 0;
        }
        else if (strcmp(argv[i], "--l2") == 0 && i + 1 < argc)
        {
            l2_mode = true;
            l2_iface = argv[++i];
            filter.enabled = true; // Use filter logic implicitly if l2
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

    if (l2_mode)
    {
        if (l2_iface.empty())
        {
            printf("Error: --l2 requires an interface name (e.g., --l2 en0)\n");
            return 1;
        }
        run_l2_mode(l2_iface, filter);
        return 0;
    }

    // Default UDP Mode

    // Initialize Winsock
#ifdef _WIN32
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }
#endif

    // Setup UDP Socket (Common for both)
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (!IS_VALIDSOCKET(sockfd))
    {
#ifdef _WIN32
        printf("socket failed with error: %ld\n", WSAGetLastError());
#else
        perror("socket");
#endif
        return 1;
    }

    // Set non-blocking
    // Set non-blocking
#ifdef _WIN32
    unsigned long mode = 1;
    if (ioctlsocket(sockfd, FIONBIO, &mode) != 0)
    {
        printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
        return 1;
    }
#else
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345); // Port we listen on
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
#ifdef _WIN32
        printf("bind failed with error: %d\n", WSAGetLastError());
#else
        perror("bind");
#endif
        return 1;
    }

    if (cli_mode)
    {
        run_cli_mode(sockfd, filter);
        CLOSE_SOCKET(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
        return 0;
    }

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    // Create window with graphics context
    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window =
        SDL_CreateWindow("Ethernet Logger (L2 Frame Viewer)", SDL_WINDOWPOS_CENTERED,
                         SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    std::vector<LogEntry> logs;
    bool auto_scroll = true;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Receive Packets
        unsigned char buffer[2048];
        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        // Loop to drain socket buffer
        while (true)
        {
            int n = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr,
                             &len);
            if (n < 0)
            {
                if (!SOCKET_AGAIN)
                {
#ifdef _WIN32
                    printf("recvfrom failed: %d\n", WSAGetLastError());
#else
                    perror("recvfrom");
#endif
                }
                break;
            }

            if (n >= 14)
            { // Minimum Ethernet Frame
                if (!packet_matches_filter(buffer, n, filter))
                    continue;

                // Check Header
                struct eth_header *eh = (struct eth_header *)buffer;
                unsigned short ether_type = ntohs(eh->h_proto);

                if (ether_type == ETH_P_LOG)
                {
                    std::string msg((char *)(buffer + 14), n - 14);
                    // Remove null terminator if present/counted
                    if (!msg.empty() && msg.back() == '\0')
                        msg.pop_back();

                    logs.push_back({ImGui::GetTime(), msg, n});
                }
            }
        }

        // GUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::Begin("Ethernet Logger", NULL,
                         ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize);

            ImGui::Text("Listening on UDP :12345 for EtherType 0x%X", ETH_P_LOG);
            ImGui::Separator();

            ImGui::BeginChild("LogRegion", ImVec2(0, 0), false,
                              ImGuiWindowFlags_HorizontalScrollbar);

            for (const auto &log : logs)
            {
                ImGui::Text("[%.3f] (%d bytes) %s", log.timestamp, log.length, log.message.c_str());
            }

            if (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();
            ImGui::End();
        }

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                     clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef _WIN32
    CLOSE_SOCKET(sockfd);
    WSACleanup();
#else
    CLOSE_SOCKET(sockfd);
#endif

    return 0;
}
