# FreeRTOS-Plus-CLI Integration

This project now uses the [FreeRTOS-Plus-CLI](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_CLI/FreeRTOS_Plus_CLI.html) for command-line processing.

## Initialization

To fetch the necessary CLI files for the first time:

```bash
./scripts/cli_init.sh
```

This script downloads `FreeRTOS_CLI.c` and `FreeRTOS_CLI.h` directly from the official FreeRTOS repository into `lib/FreeRTOS-Plus-CLI`.

## Updating

To update the submodule (e.g., after a fresh clone of this project):

```bash
./scripts/cli_update.sh
```

## Adding New Commands

To add a new command:

1. Create a callback function following the `pdCOMMAND_LINE_CALLBACK` prototype.
2. Define a `CLI_Command_Definition_t` structure.
3. Call `FreeRTOS_CLIRegisterCommand()` to register your command.

### Example: Dummy Command

A dummy command is implemented in `src/shell/cli_dummy.c` and registered during `shell_init()`.

To test it in the shell:
```bash
Shell> dummy
```

## Dependencies and Configuration

The FreeRTOS-Plus-CLI requires the following setup to function correctly:

### FreeRTOS Kernel
The CLI is designed to work with the FreeRTOS kernel. Ensure `FreeRTOS.h` is included before `FreeRTOS_CLI.h` in your source files.

### Configuration Macros
The following macros must be defined in your `FreeRTOSConfig.h`:

```c
/* FreeRTOS-Plus-CLI configuration */
#define configCOMMAND_INT_MAX_OUTPUT_SIZE 1024
#define configSUPPORT_DYNAMIC_ALLOCATION 1
```

- `configCOMMAND_INT_MAX_OUTPUT_SIZE`: Defines the size of the buffer used by the CLI to store output strings.
- `configSUPPORT_DYNAMIC_ALLOCATION`: While not strictly required by the core CLI logic, it is often necessary if you plan to use dynamic command registration or other FreeRTOS features that require heap.

### I/O Interface
You must provide a way to send strings to your output device (e.g., UART). In this project, `uart_print()` is used to transmit the generated output buffer.

## Build System

The Makefile has been updated to include:
- `lib/FreeRTOS-Plus-CLI` in the source and include paths.
