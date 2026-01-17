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

## Build System

The Makefile has been updated to include:
- `lib/FreeRTOS/FreeRTOS-Plus/Source/FreeRTOS-Plus-CLI` in the source and include paths.
