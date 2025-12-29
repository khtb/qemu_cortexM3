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
#include "uart.h"
#include <stdio.h>
#include <string.h>

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

void shell_refreshPrompt(void)
{
	uart_print(SHELL_MAIN_PROMPT);
}

void shell_execute(char *line)
{
	char *argv[SHELL_MAX_ARGS];
	int argc = 0;

	char *token = strtok(line, " ");
	while (token != NULL && argc < SHELL_MAX_ARGS)
	{
		argv[argc++] = token;
		token = strtok(NULL, " ");
	}

	if (argc == 0)
		return;

	for (uint32_t i = 0; i < SHELL_MAX_APPS; i++)
	{
		if (strcmp(argv[0], registered_apps[i]->command) == 0)
		{
			if (strcmp(argv[1], "-h") == 0)
			{
				uart_print(registered_apps[i]->help);
			}
			else
			{
				int ret = registered_apps[i]->handler(argc, argv);
				if (ret != 0)
				{
					shell_log("ERROR: Command \n");
				}
				return;
			}
			
		}
	}
	shell_log("Unknown command: \n");
}

void shell_log(const char *logString)
{
	uart_print("\nLOG: INFO: ");
	uart_print(logString);
	uart_print("\n");
	// shell_refreshPrompt();
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
