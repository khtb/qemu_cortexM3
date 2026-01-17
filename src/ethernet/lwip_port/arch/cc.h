#ifndef LWIP_ARCH_CC_H
#define LWIP_ARCH_CC_H

#include "uart.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

extern void uart_printf(const char *fmt, ...);

/* Platform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)                                                                      \
    do                                                                                             \
    {                                                                                              \
        uart_printf x;                                                                             \
    } while (0)
#define LWIP_PLATFORM_ASSERT(x)                                                                    \
    do                                                                                             \
    {                                                                                              \
        uart_puts("Assertion Failed: ");                                                           \
        uart_puts(x);                                                                              \
        while (1)                                                                                  \
            ;                                                                                      \
    } while (0)

/* Little-endian */
#define BYTE_ORDER LITTLE_ENDIAN

/* Compiler settings */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_FIELD(x) x

#endif /* LWIP_ARCH_CC_H */
