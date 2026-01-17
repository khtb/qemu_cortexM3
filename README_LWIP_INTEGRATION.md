# lwIP Integration Guide

This document outlines the steps to integrate the lightweight IP (lwIP) stack into the project.

## Step-by-Step Integration

### 1. Download lwIP
Run the initialization script to add lwIP as a submodule:
```bash
./scripts/lwip_init.sh
```

### 2. Configure lwIP
Create and customize `src/ethernet/lwip_port/lwipopts.h`. This file contains the application-specific- `net-status`: Displays lwIP interface configuration (IP, Netmask, GW).
- `ping <ip>`: Sends ICMP Echo Requests (requires network polling). Note: QEMU User Mode networking does not reply to ICMP.

### Memory Management
 settings for the lwIP stack.

### 3. Implement the Ethernet Interface (`ethernetif.c`)
This file bridges the lwIP stack with the low-level Ethernet driver (`src/ethernet/stellaris_eth.c`). It handles:
- Initialization of the hardware.
- Sending packets (`low_level_output`).
- Receiving packets (`low_level_input`).

### 4. Implement System Abstraction (`sys_arch.c`)
Since we are using FreeRTOS, lwIP needs a system abstraction layer for:
- Semaphores
- Mailboxes
- Threads/Tasks
- **Heap**: Switched to `heap_4.c` (dynamic allocation with fragmentation handling) to support `vPortFree`, which is required by lwIP and the FreeRTOS TCP/IP stack. `heap_1.c` is insufficient as it does not allow memory deallocation.

### Architecture Note
The application uses a **Multi-Task Architecture** with FreeRTOS:
- **vTaskNet**: Dedicated high-priority task that polls the Ethernet Controller (`ethernetif_input`).
- **vTaskShell**: Lower-priority task that handles the UART Command Line Interface.
- **tcpip_thread**: lwIP's internal thread for protocol processing.

## Verification

1.  **Build**: `make clean && make`
2.  **Run with Network**: `make debug_pcap` (Captures traffic to `qemu_net.pcap`)
3.  **Test Connectivity**:
    - Run `./scripts/listen_hello.sh` (Requires `netcat`).
    - You should see "Hello World" printed periodically.
4. **Telnet Access**:
    - The Telnet server listens on Guest Port 23, forwarded to Host Port **2323**.
    - Run: `telnet localhost 2323` (or `nc localhost 2323`).
    - You will see the `>` prompt and can execute shell commands like `help` or `net-status`.

### 5. Update the Build System
Add the lwIP source files and include directories to the `Makefile`.

### 6. Main Initialization
Initialize the TCP/IP stack in `main.c`:
```c
tcpip_init(NULL, NULL);
netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
netif_set_default(&gnetif);
netif_set_up(&gnetif);
```

## Maintenance
To update the lwIP stack:
```bash
git submodule update --remote lib/lwip
```

### Issues Encountered & Solutions

#### 1. FreeRTOS Queue Assertion
- **Issue**: `configASSERT` failure in `xQueueGenericCreate` (called by `sys_mbox_new`).
- **Cause**: partial `lwipopts.h` configuration led `sys_mbox_new` to request a mailbox of size 0.
- **Solution**: Defined `TCPIP_MBOX_SIZE`, `DEFAULT_*_RECVMBOX_SIZE`, and `DEFAULT_ACCEPTMBOX_SIZE` in `lwipopts.h` (set to 16). Modified `sys_mbox_new` in `sys_arch.c` to clamp size to minimum 16 if 0 is passed.

#### 2. Heap Memory Management
- **Issue**: Linker errors or runtime crashes when using `heap_1.c`.
- **Cause**: `heap_1.c` does not support `vPortFree`. lwIP requires dynamic memory deallocation (e.g., for pbufs, connections).
- **Solution**: Switched to `heap_4.c` in `Makefile`. Added missing `#include <string.h>` locally to `heap_4.c` to fix implicit `memset` warning.

#### 3. Ethernet TX Packet Corruption (Stellaris/QEMU)
- **Issue**: Telnet connection would hang (Host side `CLOSE_WAIT`), and Guest logs showed ARP Replies being sent, but no TCP traffic received.
- **Cause**: The `eth_send` function incorrectly wrote only the packet length to the first word of the TX FIFO. The QEMU Stellaris model expects the first word to contain **Length (lower 16 bits)** AND **First 2 Bytes of Payload (upper 16 bits)**. This caused the payload to be shifted by 2 bytes and zero-padded, corrupting the Destination MAC address of all outgoing packets. The Gateway ignored these corrupted ARP replies.
- **Solution**: Updated `eth_send` in `stellaris_eth.c` to correctly pack the length and the first two bytes of the `data` buffer into the first write to `MAC_DATA`.

#### 4. QEMU Networking Limitations
 but **not** deallocation (`vPortFree` is a dummy function that traps execution). lwIP requires dynamic memory freeing.
- **Solution**: Switched to `heap_4.c`, which supports memory coalescing and freeing.
- **Note**: `heap_4.c` required patching to include `<string.h>` for `memset`.

### 3. Task Starvation & Single-Tasking
- **Issue**: The shell became unresponsive when network traffic increased.
- **Cause**: The original multi-task design (separate shell and network tasks) led to context switching overhead or priority starvation where the network task consumed all CPU.
- **Solution**:
    1. Initially refactored to a **Single Task** architecture (`vTaskMain`) that polled both `ethernetif_input` and `shell_main`. This stabilized the system.
    2. Later reverted to **Multi-Task** (`vTaskNet` + `vTaskShell`) with careful yielding (`vTaskDelay`) to ensure concurrent operation without starvation.

### 4. QEMU Specific Networking
- **Issue**: `ping` commands to the gateway (`10.0.2.2`) resulted in timeout/packet loss.
- **Cause**: QEMU's User Mode Networking (SLIRP) does not implement ICMP Echo Replies for the gateway. It behaves like a firewall that only proxies TCP/UDP.
- **Verification**: Confirmed L2 connectivity by successfully running a TCP Echo Server and inspecting ARP resolution logs.

### 5. RX Packet Drop
- **Issue**: Packets sent from Host were not being processed by Guest.
- **Cause**:
    1. Promiscuous mode was disabled initially (`MAC_RCTL`).
    2. `cli_ping` command implementation was blocking the single task, preventing `ethernetif_input` from polling.
- **Solution**:
    - Enabled Promiscuous Mode in Ethernet Driver initialization.
    - Split architecture back to multi-tasking so `vTaskNet` runs in background while CLI waits.

