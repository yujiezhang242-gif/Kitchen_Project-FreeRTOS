#include "board.h"
void NMI_Handler(void) { board_panic(40); }
#if defined(__CC_ARM)
__asm void HardFault_Handler(void)
{
    TST LR, #4
    ITE EQ
    MRSEQ R0, MSP
    MRSNE R0, PSP
    MOV R1, LR
    B board_fault_entry
}
#else
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile("tst lr, #4\n ite eq\n mrseq r0, msp\n mrsne r0, psp\n mov r1, lr\n b board_fault_entry");
}
#endif
void MemManage_Handler(void) { board_panic(42); }
void BusFault_Handler(void) { board_panic(43); }
void UsageFault_Handler(void) { board_panic(44); }
void DebugMon_Handler(void) { }
