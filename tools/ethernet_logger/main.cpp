// Headers
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <ctime>
#include <iomanip>
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

void run_cli_mode(int sockfd)
{
    printf("Ethernet Logger (CLI Mode)\n");
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
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--cli") == 0)
        {
            cli_mode = true;
        }
    }

    // Setup UDP Socket (Common for both)
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
        run_cli_mode(sockfd);
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
