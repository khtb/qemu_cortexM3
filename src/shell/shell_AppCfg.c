/*
##############################################################################
#        _   __  _   _   _____   ____
#       | | / / | | | | (_   _) |  _ \
#       | |/ /  | |_| |   | |   | |_) )
#       |   <   |  _  |   | |   |  _ (
#       | |\ \  | | | |   | |   | |_) )
#       |_| \_\ |_| |_|   |_|   |____/
#
##############################################################################
******************************************************************************
 * @file shell_AppCfg.c
 * Author: KHTB
 * @brief Description of the header file
******************************************************************************/
#include "shell.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
typedef int (*Shell_AppHandler_t)(int argc, char **argv);


typedef struct {
        const char *command;
        const char *help;
        Shell_AppHandler_t handler;
} Shell_App_t;

*/
/* Centralized Shell Configuration */
#include "shell.h"
#include <string.h>

/* Forward declarations for command handlers */
extern BaseType_t prvDummyCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                  const char *pcCommandString);
extern BaseType_t prvNetStatusCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                      const char *pcCommandString);
extern BaseType_t prvPingCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString);
extern BaseType_t prvLoggerCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                   const char *pcCommandString);
extern BaseType_t prvVlanCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString);

/* Definitions for the commands */
static const CLI_Command_Definition_t xDummyCommand = {
    "dummy", "\r\ndummy:\r\n A dummy command for testing integration.\r\n", prvDummyCommand, 0};

static const CLI_Command_Definition_t xNetStatus = {
    "net-status", "\r\nnet-status:\r\n Shows network interface information\r\n",
    prvNetStatusCommand, 0};

static const CLI_Command_Definition_t xPingCommand = {
    "ping", "\r\nping <ip>:\r\n Ping an IP address\r\n", prvPingCommand, 1};

static const CLI_Command_Definition_t xLoggerCommand = {
    "log", "\r\nlog <msg>:\r\n Send a log message via Ethernet\r\n", prvLoggerCommand, 1};

static const CLI_Command_Definition_t xVlanCommand = {
    "vlan", "\r\nvlan:\r\n Interface VLAN management (show, set tagged, set untagged)\r\n", prvVlanCommand, -1};

/* The global list of commands to be registered and autocompleted */
const CLI_Command_Definition_t *const g_registered_commands[] = {&xDummyCommand, &xNetStatus,
                                                                 &xPingCommand, &xLoggerCommand,
                                                                 &xVlanCommand};

const size_t g_num_registered_commands =
    sizeof(g_registered_commands) / sizeof(CLI_Command_Definition_t *);

int App_pwd(int argc, char **argv)
{
    // 1. Use a local stack buffer instead of malloc
    char buffer[100];

    // 2. Print argc safely
    // Use %d for count; 0x%x is usually for memory addresses or bitmasks
    int len = snprintf(buffer, sizeof(buffer), "Arg count: %d\n", argc);
    if (len > 0)
    {
        uart_print(buffer);
    }

    // 3. Iterate through the argv list
    for (int i = 0; i < argc; i++)
    {
        // Format: [Index]: StringValue
        len = snprintf(buffer, sizeof(buffer), "  argv[%d]: %s\n", i, argv[i]);

        if (len > 0)
        {
            uart_print(buffer);
        }
    }
    return 0;
}

int App_Help(int argc, char **argv)
{
    uart_print("help is good");
    return 0;
}

int App_PrintData(int argc, char **argv)
{
    if (argc < 4)
    {
        uart_print("Usage: dump <addr> <count> <b|w|d>\n");
        return -1;
    }

    // 1. Parse arguments
    // strtoul is safer than atoi as it handles hex (0x prefix)
    uintptr_t addr = (uintptr_t)strtoul(argv[1], NULL, 0);
    int count = atoi(argv[2]);
    char type = argv[3][0];
    char buffer[64];

    // 2. Execute Memory Read
    for (int i = 0; i < count; i++)
    {
        uint32_t val = 0;
        if (type == 'b')
        { // Byte (8-bit)
            val = *(volatile uint8_t *)(addr + (i * 1));
            snprintf(buffer, sizeof(buffer), "0x%08X: 0x%02X\n", (unsigned int)(addr + i),
                     (unsigned int)val);
        }
        else if (type == 'w')
        { // Word (16-bit)
            val = *(volatile uint16_t *)(addr + (i * 2));
            snprintf(buffer, sizeof(buffer), "0x%08X: 0x%04X\n", (unsigned int)(addr + i * 2),
                     (unsigned int)val);
        }
        else if (type == 'd')
        { // D-Word (32-bit)
            val = *(volatile uint32_t *)(addr + (i * 4));
            snprintf(buffer, sizeof(buffer), "0x%08X: 0x%08X\n", (unsigned int)(addr + i * 4),
                     (unsigned int)val);
        }
        else
        {
            uart_print("Invalid type! Use b, w, or d.\n");
            return -1;
        }

        uart_print(buffer);
    }
    return 0;
}