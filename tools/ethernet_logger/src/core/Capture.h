#pragma once
#include "Constants.h"
#include "Filter.h"
#include "Platform.h"

class L2Capture
{
  public:
    L2Capture() = default;
    ~L2Capture();

    bool open_capture(const std::string &iface);
    void close_capture();
    int poll(std::vector<LogEntry> &logs, const Filter &filter);

  private:
    std::string interface_name;
#ifdef _WIN32
    pcap_t *adhandle = nullptr;
#elif defined(__APPLE__)
    int bpf_fd = -1;
    unsigned char *bpf_buf = nullptr;
    int bpf_buf_len = 0;
#elif defined(__linux__)
    int sock_fd = -1;
#endif

    void process_packet(const unsigned char *pkt, int len, std::vector<LogEntry> &logs,
                        const Filter &filter);
};

// Helper for UDP Mode
int open_udp_socket(int port);
