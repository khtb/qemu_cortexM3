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
 * @file shell.c
 * Author: KHTB
 * @brief Description of the header file
******************************************************************************/
#include "shell.h"
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern void vRegisterCLICommands(void);
extern void vRegisterCLICommands(void);
extern void vRegisterNetCommands(void);
extern void vRegisterPingCommand(void);

extern void vRegisterDummyCommand(void);

static void shell_refreshPrompt(void);
static int shell_autocomplete(char *buf, int *len, int max_len);

// typedef int (*Shell_AppHandler_t)(int argc, char **argv);
int Shell_Appl_Pwd(int argc, char **argv)
{
    uart_print("print working dir !!s");
    return 0;
}

void shell_init(void)
{
    vRegisterDummyCommand();
    vRegisterNetCommands();
    vRegisterPingCommand();
    uart_set_autocomplete_cb(shell_autocomplete);
    shell_refreshPrompt();
}

void shell_main(void)
{
    char line[100];
    int x = uart_readLine(line, 64);
    if (x != -1)
    {
        shell_execute(line);
        shell_refreshPrompt();
    }
}

void shell_refreshPrompt(void) { uart_print(SHELL_MAIN_PROMPT); }

void shell_execute(char *line)
{
    char pcOutputString[128];
    BaseType_t xReturned;

    /* First, try processing via FreeRTOS-Plus-CLI */
    do
    {
        /* Get the next string to print. */
        xReturned = FreeRTOS_CLIProcessCommand(line, pcOutputString, sizeof(pcOutputString));

        /* Write the generated string to the UART. */
        uart_print(pcOutputString);

    } while (xReturned != pdFALSE);

    /* Fallback to legacy shell apps if needed (optional)
       Note: FreeRTOS-Plus-CLI will return "Command not recognized" if not found.
       If you want to keep legacy apps separately, you might need to check the output.
       For now, we assume migration or that FreeRTOS-Plus-CLI handles it.
    */
}

void shell_log(const char *logString)
{
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "LOG: INFO: %s", logString);
    uart_log_and_refresh(buffer, SHELL_MAIN_PROMPT);
}

int shell_autocomplete(char *buf, int *len, int max_len)
{
    int current_len = *len;
    int matches = 0;
    int match_idx = -1;
    // Iterate through all registered apps
    for (uint32_t i = 0; i < SHELL_MAX_APPS; i++)
    {
        // Check if command starts with buf
        if (strncmp(registered_apps[i]->command, buf, current_len) == 0)
        {
            matches++;
            match_idx = i;
        }
    }

    // If exactly one match found, autocomplete it
    if (matches == 1 && match_idx != -1)
    {
        const char *cmd = registered_apps[match_idx]->command;
        int cmd_full_len = strlen(cmd);

        if (cmd_full_len < max_len)
        {
            strcpy(buf, cmd);
            *len = cmd_full_len;
            return 1; // Completed
        }
    }
    return 0;
}
