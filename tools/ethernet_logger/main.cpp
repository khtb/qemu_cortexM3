#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <arpa/inet.h>
#include <ctime>
#include <fcntl.h>
#include <iomanip>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

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
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                perror("recvfrom");
                usleep(10000); // 10ms wait on error to avoid busy loop if something breaks
            }
            else
            {
                usleep(1000); // 1ms wait to lower CPU in CLI polling
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
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("socket");
        return 1;
    }

    // Set non-blocking
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(12345); // Port we listen on
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind");
        return 1;
    }

    if (cli_mode)
    {
        run_cli_mode(sockfd);
        close(sockfd);
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
            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cliaddr, &len);
            if (n < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    perror("recvfrom");
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

    return 0;
}
