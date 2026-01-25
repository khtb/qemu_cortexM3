# QEMU Cortex-M3 Ethernet Project

## Overview
This project simulates a Cortex-M3 (Stellaris LM3S6965) with Ethernet support on QEMU.
It features FreeRTOS, LwIP, a CLI shell, and a custom Ethernet Logger.

## Ethernet Logger
A custom Logger Task sends debug messages as raw Ethernet L2 frames (EtherType 0x88B5).
A companion GUI tool (`tools/ethernet_logger`) receives and displays these logs.

### Usage
1. Build and run the GUI tool:
   ```bash
   cd tools/ethernet_logger && make && ./eth_logger
   ```
2. Run QEMU with logger support (in a separate terminal):
   ```bash
   make run_logger
   ```
3. Use the `log <msg>` shell command in QEMU to send custom logs.

## Shell Commands
- `net-status`: Show IP/MAC info.
- `ping <ip>`: Ping an IP address.
- `log <msg>`: Send a log message to the logger tool.
- `help`: List commands.

## Build & Run (Standard)
LM3S6965EVB emulation includes the following devices:

Cortex-M3 CPU core.  
256k Flash and 64k SRAM.  
Timers, UARTs, ADC, I2C and SSI interfaces.  
OSRAM Pictiva 128x64 OLED with SSD0323 controller connected via SSI.  

in Qemu Repo


for RTOS files in repo, can be copied from RTOS repo into wrking dir , with scripts/


TASKS :  
- [x] integrate RTOS  
- [x] set up gdb debug in vscode  
- [x] test uart with Qemu terminal 
- [ ] Ethernet driver integration
- [ ] create py file to test traffic between host <-> qemu 



### Build and run:
---
to start QEMU 
```
make run
```

### Environmet 
---
.vscode settings to launch debug and start QEMU
debug uses Cortex-Debug vscode extension.

make sure that `arm-none-eabi-gdb` is configured in cortex-debug settings.

example 
``` json
  "cortex-debug.armToolchainPath": "/opt/homebrew/bin",
  "cortex-debug.gdbPath": "/opt/homebrew/bin/arm-none-eabi-gdb",
  "cortex-debug.armToolchainPrefix": "arm-none-eabi",
  "cortex-debug.openocdPath": "/opt/homebrew/bin/openocd"
```


