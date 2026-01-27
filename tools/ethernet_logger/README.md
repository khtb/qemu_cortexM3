# Ethernet Logger Tool

This tool visualizes Ethernet L2 frames sent from the QEMU simulation via UDP, or captures raw packets directly from a network interface (L2 Mode). It uses [Dear ImGui](https://github.com/ocornut/imgui) for the UI and SDL2 for window/input.

## Features
-   **UDP Mode**: Listens for encapsulated frames (default).
-   **L2 Raw Mode**: Captures raw Ethernet frames from a physical interface (`--l2`).
-   **Filtering**: Filter by Source/Dest MAC and EtherType.
-   **Cross-Platform**: Works on macOS, Linux, and Windows.

## Prerequisites

### macOS / Linux
-   **SDL2**: `brew install sdl2` (macOS) or `sudo apt install libsdl2-dev` (Linux).
-   **Compiler**: clang or gcc.

### Windows (MinGW)
-   **MinGW-w64**: Ensure `g++` is in your PATH.
-   **Npcap**: Install the [Npcap Driver](https://npcap.com/) (required for L2 capture).
-   **SDKs**: run `./setup_windows_deps.sh` to download SDL2 and Npcap SDK.

## Building

### macOS / Linux
```bash
make
```

### Windows (MinGW)
1.  Run the setup script once:
    ```bash
    ./setup_windows_deps.sh
    ```
2.  Build:
    ```bash
    make windows
    ```

## Usage

### 1. List Available Interfaces
To find the correct interface name for L2 capture, use the `--list` argument.

```bash
# macOS/Linux
./eth_logger --list

# Windows
eth_logger.exe --list
```
*Output Example (Windows):*
```
Name: \Device\NPF_{F46D0...}
      Description: Intel(R) Ethernet Connection
      Address (IPv4): 192.168.1.10
```

### 2. Run in L2 Capture Mode
Capture raw frames from a specific interface. Root/Admin privileges are usually required.

**Syntax:**
```bash
# General
sudo ./eth_logger --cli --l2 <INTERFACE_NAME> [FILTERS]
```

**Examples:**

*   **macOS (Capture on en0):**
    ```bash
    sudo ./eth_logger --cli --l2 en0
    ```

*   **Linux (Capture on eth0):**
    ```bash
    sudo ./eth_logger --cli --l2 eth0
    ```

*   **Windows (Capture on specific GUID):**
    *Use quotes for device names containing special characters.*
    ```bash
    eth_logger.exe --cli --l2 "\Device\NPF_{F46D0436-A05F-42A9-93C0-36C38A13C686}"
    ```

*   **Custom Device Path:**
    If you have a specific device path (e.g., specific BPF device), you can pass it directly:
    ```bash
    sudo ./eth_logger --cli --l2 /dev/bpf0
    ```

### 3. Filtering
You can filter traffic by Source MAC, Destination MAC, or EtherType.

-   `--src <MAC>`: Source MAC address (e.g., `00:11:22:33:44:55`)
-   `--dst <MAC>`: Destination MAC address
-   `--type <HEX>`: EtherType (e.g., `0x0800` for IPv4)

**Example:**
Capture only IPv4 traffic from `AA:BB:CC:DD:EE:FF`:
```bash
sudo ./eth_logger --cli --l2 en0 --type 0x0800 --src AA:BB:CC:DD:EE:FF
```

### 4. UDP Mode (Legacy/QEMU Default)
Listens on UDP port `12345`.
```bash
./eth_logger --cli
```
