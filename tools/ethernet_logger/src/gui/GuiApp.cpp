#include "GuiApp.h"
#include "../core/Capture.h"
#include "../core/Device.h"

// ImGui & SDL Headers (Assume include paths are set correctly in Makefile)
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <vector>

namespace GUI
{

GuiApp::GuiApp(const Core::AppConfig &config) : m_config(config) {}

int GuiApp::run()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    const char *glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

    SDL_WindowFlags window_flags =
        (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Ethernet Logger", SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // State
    std::vector<LogEntry> logs;
    bool auto_scroll = true;
    int current_mode = m_config.l2_mode ? 1 : 0;

    // L2 State
    L2Capture l2_capture;
    bool l2_capturing = false;
    auto devices = get_device_list();
    int selected_device_idx = -1;
    char ethertype_buf[16] = "0x88B5"; // Default
    Filter filter = m_config.filter;   // Local copy to modify

    // UDP State
    int udp_sockfd = -1;
    if (current_mode == 0)
    {
        udp_sockfd = open_udp_socket(12345);
    }

    // Pre-select iface if provided
    if (!m_config.iface.empty())
    {
        for (size_t i = 0; i < devices.size(); i++)
        {
            if (devices[i].name == m_config.iface)
            {
                selected_device_idx = i;
                break;
            }
        }
    }
    // Auto-start if L2 mode requested via args
    if (m_config.l2_mode && selected_device_idx >= 0)
    {
        if (l2_capture.open_capture(devices[selected_device_idx].name))
        {
            l2_capturing = true;
        }
    }

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
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

        // Logic
        if (current_mode == 0)
        {
            if (udp_sockfd >= 0)
            {
                unsigned char buffer[2048];
                struct sockaddr_in cliaddr;
                socklen_t len = sizeof(cliaddr);
                while (true)
                {
                    int n = recvfrom(udp_sockfd, (char *)buffer, sizeof(buffer), 0,
                                     (struct sockaddr *)&cliaddr, &len);
                    if (n < 0)
                        break;

                    if (n >= 14)
                    {
                        if (packet_matches_filter(buffer, n, filter))
                        {
                            struct eth_header *eh = (struct eth_header *)buffer;
                            unsigned short ether_type = ntohs(eh->h_proto);
                            if (m_config.filter.has_type ? (ether_type == m_config.filter.type)
                                                         : (ether_type == ETH_P_LOG))
                            {
                                std::string msg((char *)(buffer + 14), n - 14);
                                if (!msg.empty() && msg.back() == '\0')
                                    msg.pop_back();
                                logs.push_back({ImGui::GetTime(), msg, n});

                                if (m_config.payload_only)
                                    printf("%s\n", msg.c_str());
                                else
                                    printf("[%.3f] (%d bytes) %s\n", ImGui::GetTime(), n,
                                           msg.c_str());
                                fflush(stdout);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            if (l2_capturing)
            {
                l2_capture.poll(logs, filter);
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

            ImGui::Text("Capture Mode:");
            ImGui::SameLine();
            if (ImGui::RadioButton("UDP Listener", &current_mode, 0))
            {
                if (l2_capturing)
                {
                    l2_capture.close_capture();
                    l2_capturing = false;
                }
                if (udp_sockfd < 0)
                    udp_sockfd = open_udp_socket(12345);
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("L2 Raw Capture", &current_mode, 1))
            {
                if (udp_sockfd >= 0)
                {
                    CLOSE_SOCKET(udp_sockfd);
                    udp_sockfd = -1;
                }
            }

            if (current_mode == 0)
            {
                if (m_config.filter.has_type)
                    ImGui::Text("Listening on UDP :12345 for EtherType 0x%04X",
                                m_config.filter.type);
                else
                    ImGui::Text("Listening on UDP :12345 for EtherType 0x88B5");
            }
            else
            {
                std::string current_dev_name =
                    (selected_device_idx >= 0 && selected_device_idx < (int)devices.size())
                        ? devices[selected_device_idx].display_name
                        : "Select Interface";

                if (ImGui::BeginCombo("Interface", current_dev_name.c_str()))
                {
                    for (int i = 0; i < (int)devices.size(); i++)
                    {
                        bool is_selected = (selected_device_idx == i);
                        if (ImGui::Selectable(devices[i].display_name.c_str(), is_selected))
                        {
                            selected_device_idx = i;
                            if (l2_capturing)
                            {
                                l2_capturing = false;
                                l2_capture.close_capture();
                            }
                        }
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                static bool use_ethertype = true;
                ImGui::Checkbox("Filter EtherType", &use_ethertype);
                if (use_ethertype)
                {
                    ImGui::Indent();
                    ImGui::InputText("EtherType (Hex)", ethertype_buf, sizeof(ethertype_buf));
                    ImGui::Unindent();
                }

                static bool use_src = false;
                static char src_buf[18] = "";
                ImGui::Checkbox("Filter Source MAC", &use_src);
                if (use_src)
                {
                    ImGui::Indent();
                    ImGui::InputText("Source MAC", src_buf, sizeof(src_buf));
                    ImGui::Unindent();
                }

                static bool use_dst = false;
                static char dst_buf[18] = "";
                ImGui::Checkbox("Filter Dest MAC", &use_dst);
                if (use_dst)
                {
                    ImGui::Indent();
                    ImGui::InputText("Dest MAC", dst_buf, sizeof(dst_buf));
                    ImGui::Unindent();
                }

                if (!l2_capturing)
                {
                    if (ImGui::Button("Start Capture"))
                    {
                        if (selected_device_idx >= 0)
                        {
                            filter.enabled = true; // IMPORTANT: Enable filtering logic
                            filter.has_type = false;
                            filter.has_src = false;
                            filter.has_dst = false;

                            bool valid_params = true;

                            if (use_ethertype)
                            {
                                unsigned short type;
                                if (parse_hex(ethertype_buf, &type))
                                {
                                    filter.has_type = true;
                                    filter.type = type;
                                }
                                else
                                {
                                    printf("Invalid EtherType format\n");
                                    valid_params = false;
                                }
                            }

                            if (use_src)
                            {
                                if (parse_mac(src_buf, filter.src))
                                {
                                    filter.has_src = true;
                                }
                                else
                                {
                                    printf("Invalid Source MAC format\n");
                                    valid_params = false;
                                }
                            }

                            if (use_dst)
                            {
                                if (parse_mac(dst_buf, filter.dst))
                                {
                                    filter.has_dst = true;
                                }
                                else
                                {
                                    printf("Invalid Dest MAC format\n");
                                    valid_params = false;
                                }
                            }

                            if (valid_params)
                            {
                                if (l2_capture.open_capture(devices[selected_device_idx].name))
                                {
                                    l2_capturing = true;
                                }
                                else
                                {
                                    printf("Failed to open adapter\n");
                                }
                            }
                        }
                    }
                }
                else
                {
                    if (ImGui::Button("Stop Capture"))
                    {
                        l2_capture.close_capture();
                        l2_capturing = false;
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Capturing...");
                }
            }

            ImGui::Separator();
            ImGui::BeginChild("LogRegion", ImVec2(0, 0), false,
                              ImGuiWindowFlags_HorizontalScrollbar);
            for (const auto &log : logs)
            {
                if (m_config.payload_only)
                    ImGui::Text("%s", log.message.c_str());
                else
                    ImGui::Text("[%.3f] (%d bytes) %s", log.timestamp, log.length,
                                log.message.c_str());
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

    if (l2_capturing)
        l2_capture.close_capture();
    if (udp_sockfd >= 0)
        CLOSE_SOCKET(udp_sockfd);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
} // namespace GUI
