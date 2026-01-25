#include "FreeRTOS.h"

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"
#include "logger_task.h"

#include <stdio.h>
#include <string.h>

BaseType_t prvLoggerCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                            const char *pcCommandString)
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;

    /* Parse parameters */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);

    if (pcParameter != NULL)
    {
        /* We can't use the parameter string directly if it's not null terminated,
           so copy it or use length. logger_log handles null-terminated strings. */
        char logMsg[128];
        if (xParameterStringLength >= sizeof(logMsg))
        {
            xParameterStringLength = sizeof(logMsg) - 1;
        }
        strncpy(logMsg, pcParameter, xParameterStringLength);
        logMsg[xParameterStringLength] = '\0';

        logger_log(logMsg);
        snprintf(pcWriteBuffer, xWriteBufferLen, "Logged: %s\r\n", logMsg);
    }
    else
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Usage: log <message>\r\n");
    }

    return pdFALSE;
}
