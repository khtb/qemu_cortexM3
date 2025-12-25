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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>




/* 
typedef int (*Shell_AppHandler_t)(int argc, char **argv);


typedef struct {
	const char *command;
	const char *help;
	Shell_AppHandler_t handler;
} Shell_App_t;

*/
int App_Help(int argc, char **argv);
int App_pwd(int argc, char **argv);
int App_PrintData(int argc, char **argv);

const Shell_App_t app_help = {"help", "Display available commands", App_Help};
const Shell_App_t pwd = {"pwd", "you know pwd and so", App_pwd};
const Shell_App_t dumpData = {"dump","print data at address", App_PrintData};

const Shell_App_t* registered_apps[SHELL_MAX_APPS] = {&app_help,&pwd,&dumpData};




int App_pwd(int argc, char **argv)
{
	// 1. Use a local stack buffer instead of malloc
    char buffer[100]; 

    // 2. Print argc safely
    // Use %d for count; 0x%x is usually for memory addresses or bitmasks
    int len = snprintf(buffer, sizeof(buffer), "Arg count: %d\n", argc);
    if (len > 0) {
        uart_print(buffer);
    }

    // 3. Iterate through the argv list
    for (int i = 0; i < argc; i++) {
        // Format: [Index]: StringValue
        len = snprintf(buffer, sizeof(buffer), "  argv[%d]: %s\n", i, argv[i]);
        
        if (len > 0) {
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
			snprintf(buffer, sizeof(buffer), "0x%08X: 0x%02X\n", (unsigned int)(addr + i), (unsigned int)val);
		}
		else if (type == 'w')
		{ // Word (16-bit)
			val = *(volatile uint16_t *)(addr + (i * 2));
			snprintf(buffer, sizeof(buffer), "0x%08X: 0x%04X\n", (unsigned int)(addr + i * 2), (unsigned int)val);
		}
		else if (type == 'd')
		{ // D-Word (32-bit)
			val = *(volatile uint32_t *)(addr + (i * 4));
			snprintf(buffer, sizeof(buffer), "0x%08X: 0x%08X\n", (unsigned int)(addr + i * 4), (unsigned int)val);
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