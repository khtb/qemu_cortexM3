/*
################################################################################
#        _   __  _   _   _____   ____
#       | | / / | | | | (_   _) |  _ \
#       | |/ /  | |_| |   | |   | |_) )
#       |   <   |  _  |   | |   |  _ (
#       | |\ \  | | | |   | |   | |_) )
#       |_| \_\ |_| |_|   |_|   |____/
#
################################################################################
********************************************************************************
 * @file shell.h
 * Author: KHTB
 * @brief Description of the header file
*******************************************************************************/
#ifndef SHELL_H_
#define SHELL_H_

#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ANSI Escape Sequences */
#define ANSI_CLEAR_LINE "\r\x1b[K"

#define SHELL_MAIN_PROMPT "Shell> "
#define SHELL_MAX_ARGS 10u
#define SHELL_MAX_COMMANDS 20

    typedef void (*shell_output_func_t)(const char *str);

    extern void shell_init(void);
    extern void shell_main(void);
    extern void shell_execute(char *line);
    extern void shell_process(char *line, shell_output_func_t out_func);
    extern void shell_log(const char *logString);
    extern int shell_autocomplete(char *buf, int *len, int max_len);

    /* Centralized command list (Defined in shell_AppCfg.c) */
    extern const CLI_Command_Definition_t *const g_registered_commands[];
    extern const size_t g_num_registered_commands;

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H_ */