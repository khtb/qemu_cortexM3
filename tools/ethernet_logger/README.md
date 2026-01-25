# Ethernet Logger Tool

This tool visualizes Ethernet L2 frames sent from the QEMU simulation via UDP.
It uses [Dear ImGui](https://github.com/ocornut/imgui) for the UI and SDL2 for window/input.

## Prerequisites
- macOS or Linux
- SDL2 (Installed via Homebrew on Mac: `brew install sdl2`)
- C++ Compiler (clang/gcc)

## Building
Run `make` in this directory:
```bash
make
```

## Running
1. Run the logger tool first (or in parallel):
   ```bash
   ./eth_logger
   ```
   Or run in CLI mode:
   ```bash
   ./eth_logger --cli
   ```
   It listens on UDP port `12345` for packets with EtherType `0x88B5`.

2. Run the Firmware in QEMU (from the project root):
   ```bash
   make run_logger
   ```

The logger should display incoming messages from the firmware.
