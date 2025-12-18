
#include <stdint.h>
#include "uart.h"


void Reset_Handler(void);
void Default_Handler(void);

void NMI_Handler(void)       __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));



/* linker symbols */
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss, _estack;
extern void main(void);



extern void __main(void);
/* vector table at 0x00000000 */
void MemManage_Handler()
{

}
void BusFault_Handler()
{

}
void UsageFault_Handler(void)
{

}
void DebugMon_Handler(void)
{

}

extern void SVC_Handler();
extern void PendSV_Handler();
extern void SysTick_Handler();
__attribute__((section(".isr_vector"))) void (*const vector_table[])(void) = {
    (void (*)(void))(&_estack), /* initial SP */
     Reset_Handler,                            /*     Reset Handler */
     NMI_Handler,                              /* -14 NMI Handler */
     HardFault_Handler,                        /* -13 Hard Fault Handler */
     MemManage_Handler,                        /* -12 MPU Fault Handler */
     BusFault_Handler,                         /* -11 Bus Fault Handler */
     UsageFault_Handler,                       /* -10 Usage Fault Handler */
     0,                                        /*     Reserved */
     0,                                        /*     Reserved */
     0,                                        /*     Reserved */
     0,                                        /*     Reserved */
    SVC_Handler,                              /*  -5 SVC Handler */
    DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
    0,                                        /*     Reserved */
    PendSV_Handler,                           /*  -2 PendSV Handler */
    SysTick_Handler,                          /*  -1 SysTick Handler */ 
    0,0,0,0,0,
    uart_irq, // UART0 interrupt
};
extern void __libc_init_array();
extern void __DSB(void);

#define SCB_CPACR  (*(volatile uint32_t *)0xE000ED88)


void Reset_Handler(void) {

    #if 0
    __libc_init_array();
    #else
    /* copy .data from FLASH to RAM */
    uint32_t *src = &_sidata, *dst = &_sdata;
    while (dst < &_edata) *dst++ = *src++;
    /* zero .bss */
    for (dst = &_sbss; dst < &_ebss; ) *dst++ = 0;
//     SCB->CPACR |= (0xF << 20);  // Enable full access to CP10 and CP11 (FPU)
    SCB_CPACR |= (0xF << 20);

// __DSB();
// __ISB();
    /* jump to main */
    (void)main();
    for(;;);
    #endif
}

void Default_Handler(void) { for(;;); }