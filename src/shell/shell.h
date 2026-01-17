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

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ANSI Escape Sequences */
#define ANSI_CLEAR_LINE "\r\x1b[K"

#define SHELL_MAIN_PROMPT "Shell> "
#define SHELL_MAX_ARGS 10u
#define SHELL_MAX_APPS 10

    typedef void (*shell_output_func_t)(const char *str);

    extern void shell_init(void);
    extern void shell_main(void);
    extern void shell_execute(char *line);
    extern void shell_process(char *line, shell_output_func_t out_func);
    extern void shell_log(const char *logString);

    /**
     * @brief Command handler function pointer
     */
    typedef int (*Shell_AppHandler_t)(int argc, char **argv);

    /**
     * @brief Shell Application (Command) Descriptor
     */
    typedef struct
    {
        const char *command;
        const char *help;
        Shell_AppHandler_t handler;
    } Shell_App_t;

    extern const Shell_App_t *registered_apps[SHELL_MAX_APPS];

#ifdef __cplusplus
}
#endif

#endif /* SHELL_H_ */