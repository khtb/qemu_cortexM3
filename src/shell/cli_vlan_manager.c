#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "uart.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_VLANS 4096

typedef enum {
    VLAN_UNCONFIGURED = 0,
    VLAN_TAGGED,
    VLAN_UNTAGGED
} VlanState;

static VlanState vlan_table[MAX_VLANS] = {0};
static int vlan_show_idx = 0;

BaseType_t prvVlanCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *pcParam1, *pcParam2, *pcParam3;
    BaseType_t xParam1Len, xParam2Len, xParam3Len;

    pcParam1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParam1Len);

    if (pcParam1 != NULL)
    {
        if (strncmp(pcParam1, "show", xParam1Len) == 0 && xParam1Len == 4)
        {
            int offset = 0;
            if (vlan_show_idx == 0)
            {
                bool has_vlans = false;
                for (int i=0; i<MAX_VLANS; i++) {
                    if (vlan_table[i] != VLAN_UNCONFIGURED) has_vlans = true;
                }
                
                if (!has_vlans) {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "No VLANs configured.\r\n");
                    return pdFALSE;
                }
                offset = snprintf(pcWriteBuffer, xWriteBufferLen, "Configured VLANs:\r\n");
            }

            while (vlan_show_idx < MAX_VLANS)
            {
                if (vlan_table[vlan_show_idx] != VLAN_UNCONFIGURED)
                {
                    const char *s = (vlan_table[vlan_show_idx] == VLAN_TAGGED) ? "TAGGED" : "UNTAGGED";
                    offset += snprintf(pcWriteBuffer + offset, xWriteBufferLen - offset,
                                       " VLAN %d: %s\r\n", vlan_show_idx, s);
                    vlan_show_idx++;
                    return pdTRUE; /* Still potentially more to print */
                }
                vlan_show_idx++;
            }
            vlan_show_idx = 0;
            if (offset == 0) pcWriteBuffer[0] = '\0';
            return pdFALSE; /* Finished printing */
        }
        else if (strncmp(pcParam1, "set", xParam1Len) == 0 && xParam1Len == 3)
        {
            vlan_show_idx = 0; // Reset state

            pcParam2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParam2Len);
            pcParam3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParam3Len);

            if (pcParam2 != NULL && pcParam3 != NULL)
            {
                char vid_str[16];
                int vid = -1;
                if (xParam3Len < (BaseType_t)sizeof(vid_str)) {
                    strncpy(vid_str, pcParam3, xParam3Len);
                    vid_str[xParam3Len] = '\0';
                    vid = atoi(vid_str);
                }

                if (vid >= 0 && vid < MAX_VLANS)
                {
                    if (strncmp(pcParam2, "tagged", xParam2Len) == 0 && xParam2Len == 6)
                    {
                        vlan_table[vid] = VLAN_TAGGED;
                        snprintf(pcWriteBuffer, xWriteBufferLen, "VLAN %d set correctly to TAGGED\r\n", vid);
                    }
                    else if (strncmp(pcParam2, "untagged", xParam2Len) == 0 && xParam2Len == 8)
                    {
                        vlan_table[vid] = VLAN_UNTAGGED;
                        snprintf(pcWriteBuffer, xWriteBufferLen, "VLAN %d set correctly to UNTAGGED\r\n", vid);
                    }
                    else
                    {
                        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Unknown state. Use 'tagged' or 'untagged'.\r\n");
                    }
                }
                else
                {
                    snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Invalid VLAN ID.\r\n");
                }
            }
            else
            {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Usage: vlan set <tagged|untagged> <vid>\r\n");
            }
            return pdFALSE;
        }
        else
        {
            vlan_show_idx = 0;
            snprintf(pcWriteBuffer, xWriteBufferLen, "Usage: vlan <show | set>\r\n");
            return pdFALSE;
        }
    }
    
    vlan_show_idx = 0;
    snprintf(pcWriteBuffer, xWriteBufferLen, "Usage:\r\n vlan show\r\n vlan set <tagged|untagged> <vid>\r\n");
    return pdFALSE;
}
