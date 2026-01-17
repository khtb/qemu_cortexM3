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

## Issues Encountered & Solutions

### 1. FreeRTOS Queue Assertion
- **Issue**: Application crashed with `configASSERT` in `xQueueGenericCreate`.
- **Cause**: lwIP was requesting mailboxes (queues) with size `0`, interpreting it as "use default", but `xQueueCreate` requires a positive size.
- **Solution**:
  - Modified `sys_mbox_new` in `sys_arch.c` to enforce a default minimum size if `size <= 0`.
  - Defined explicit default mailbox sizes (`DEFAULT_TCP_RECVMBOX_SIZE`, etc.) in `lwipopts.h`.

### 2. Heap Memory Management
- **Issue**: `vPortFree` caused assertion failures / crashes.
- **Cause**: The project was using `heap_1.c` (FreeRTOS default), which only supports allocation but **not** deallocation (`vPortFree` is a dummy function that traps execution). lwIP requires dynamic memory freeing.
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

