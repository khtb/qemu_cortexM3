# lwIP Integration Debug Log

This file documents every attempt, failure, and fix during the lwIP integration process.

## [2026-01-17 01:05] Phase 1: Planning and Research

### Issue 01: Submodule Addition
- **Thinking**: Adding lwIP as a submodule is the cleanest way to manage external dependencies while allowing for potential upstream updates.
- **Action**: Created `scripts/lwip_init.sh` using `git submodule add`.
- **Result**: Initializing the repository.

## [2026-01-17 01:10] Phase 2: Implementation of Porting Layer

### Issue 04: Porting Directory Structure
- **Thinking**: lwIP expected a certain structure for its porting files. I'll put them in `src/ethernet/lwip_port`.
- **Action**: Created the directory.

### Issue 05: `lwipopts.h` Configuration
- **Thinking**: This is the heart of lwIP configuration. I need to enable NO_SYS (for now, or set it to 0 for FreeRTOS) and other basic features.
- **Decision**: I'll target FreeRTOS integration from the start, so `NO_SYS = 0`. I'll need to define thread priorities.

### Issue 06: `ethernetif.c` Boilerplate
- **Thinking**: I'll use the standard `ethernetif.c` template from lwIP `contrib` or standard examples and adapt it to `stellaris_eth.c`.
- **Challenge**: The `stellaris_eth.c` driver is polling-based and uses `MAC_DATA` to drain/send. I need to ensure buffers are properly handled between `pbuf` and the hardware.

### Issue 07: Makefile and Include Paths
- **Thinking**: lwIP has a complex structure. I need to include multiple subdirectories for core, ipv4, and api.
- **Action**: Updated `Makefile` with `lib/lwip/src/core`, `lib/lwip/src/core/ipv4`, `lib/lwip/src/api`, and `lib/lwip/src/netif`.
- **Challenge**: Many lint errors appeared regarding missing headers. This is likely due to the "multi-layered" include structure of lwIP (e.g., `arch/cc.h` needing `uart.h`). I need to ensure all base directories are in the include path.
- **Fix**: Added `src` and `src/ethernet/lwip_port` explicitly to `INCDIR`.

## [2026-01-17 01:20] Phase 3: Initial Build Attempt

### Trial 02: `sys_mutex_t` and `sys_prot_t` errors
- **Error**: `unknown type name 'sys_mutex_t'` and `unknown type name 'sys_prot_t'` in `lwip/sys.h`.
- **Reason**: lwIP core requires these types to be defined in `sys_arch.h` when `NO_SYS=0`.
- **Fix**: Defined `sys_mutex_t` as `SemaphoreHandle_t` and `sys_prot_t` as `uint32_t` in `src/ethernet/lwip_port/arch/sys_arch.h`.

### Trial 03: Include recursion and ordering
- **Error**: `lwip/etharp.h` not found in `sys_arch.h`.
- **Thinking**: I mistakenly added lwIP headers to `sys_arch.h` which created a circular dependency or just failed because include paths weren't perfect yet.
- **Fix**: Cleaned up `sys_arch.h` to only include FreeRTOS headers and the hardware header. Removed redundant lwIP includes from `sys_arch.h`.

### Trial 05: Missing `errno` and sockets errors
- **Error**: `ENOPROTOOPT`, `EFAULT`, `EBADF`, and `errno` undeclared in `lwip/api/sockets.c`.
- **Reason**: The C library headers were not fully visible to lwIP or lwIP was not configured to use the toolchain's `errno.h`.
- **Fix**: Added `#include <errno.h>` to `src/ethernet/lwip_port/arch/cc.h`.
