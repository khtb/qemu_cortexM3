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

// typedef int (*Shell_AppHandler_t)(int argc, char **argv);
int Shell_Appl_Pwd(int argc, char **argv)
{

	uart_print("print working dir !!s");
}
/* ANSI Escape Codes */
#define ANSI_CLEAR_LINE "\r\x1b[K"
#define ANSI_UP_CHAR    "\r\x1b[K"

void shell_init(void)
{
	shell_refreshPrompt();
}

void shell_main(void)
{
	char line[100];
	int x = uart_readLine(line,64);
	if (x!= -1)
	{
		shell_execute(line);
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
			if (strcmp(argv[1] ,"-h") == 0)
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
				shell_refreshPrompt();
				return;
			}
		}
	}
	shell_log("Unknown command: \n");
	shell_refreshPrompt();	
}



void shell_log(const char *logString)
{
	uart_print(logString);
}