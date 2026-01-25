#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>

/* The function that implements the dummy command. */
BaseType_t prvDummyCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                           const char *pcCommandString);

/* Structure that defines the "dummy" command line command. */
static const CLI_Command_Definition_t xDummyCommand = {
    "dummy", /* The command string to type. */
    "\r\ndummy:\r\n A dummy command for testing integration.\r\n",
    prvDummyCommand, /* The function to run. */
    0                /* No parameters are expected. */
};

BaseType_t prvDummyCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void)pcCommandString;
    strncpy(pcWriteBuffer, "Dummy command executed successfully via FreeRTOS-Plus-CLI!\r\n",
            xWriteBufferLen);

    /* There is no more data to return after this string, so return pdFALSE. */
    return pdFALSE;
}
