#include "board.h"
#include "board_config.h"
#include "dht_decoder.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
static uint32_t reset_flags;
static uint16_t adc_buffer[32];
static uint8_t rx_dma[256],rx_ring[512],tx_dma[138];
static volatile unsigned rx_head,rx_tail;
static unsigned dma_position;
static volatile uint32_t rx_drops,rx_errors;
static void gpio(GPIO_TypeDef *p,uint16_t pin,GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef c; c.GPIO_Pin=pin; c.GPIO_Mode=mode; c.GPIO_Speed=GPIO_Speed_2MHz; GPIO_Init(p,&c);
}
#define CYCLE_CTRL (*(volatile uint32_t *)0xE0001000u)
#define CYCLE_COUNT (*(volatile uint32_t *)0xE0001004u)
static uint32_t wall_ms,previous_cycles,cycle_remainder;
uint32_t board_cycles(void) { return CYCLE_COUNT; }
uint32_t board_cycles_per_ms(void) { return SystemCoreClock/1000u; }
uint32_t board_millis(void)
{
    uint32_t now,delta,unit,value;
    /* DWT counts CPU cycles through Flash stalls. Unlike SysTick pending bits,
     * it does not collapse multiple milliseconds into one after an erase.
     * At 72MHz this must run at least once per 59 seconds; Health runs at 0.5s. */
    taskENTER_CRITICAL();
    now=CYCLE_COUNT; delta=now-previous_cycles; previous_cycles=now;
    unit=SystemCoreClock/1000u;
    wall_ms+=delta/unit; cycle_remainder+=delta%unit;
    if(cycle_remainder>=unit) { wall_ms++; cycle_remainder-=unit; }
    value=wall_ms; taskEXIT_CRITICAL(); return value;
}
void board_delay_us(uint16_t us)
{
    uint16_t start=(uint16_t)TIM4->CNT;
    while((uint16_t)((uint16_t)TIM4->CNT-start)<us) { }
}
static volatile DhtDecoder dht;
void EXTI9_5_IRQHandler(void)
{
    if(EXTI_GetITStatus(EXTI_Line8)!=RESET) {
        uint16_t at=(uint16_t)TIM4->CNT;
        bool high=GPIO_ReadInputDataBit(BOARD_DHT_PORT,BOARD_DHT_PIN)!=0;
        EXTI_ClearITPendingBit(EXTI_Line8);
        dht_decoder_edge(&dht,high,at);
        if(dht.complete || dht.failed) EXTI->IMR&=~EXTI_Line8;
    }
}
bool board_dht_read(int16_t *temperature,uint16_t *humidity)
{
    uint32_t start;
    EXTI->IMR&=~EXTI_Line8;
    GPIO_ResetBits(BOARD_DHT_PORT,BOARD_DHT_PIN); vTaskDelay(pdMS_TO_TICKS(20));
    NVIC_DisableIRQ(EXTI9_5_IRQn); dht_decoder_start(&dht);
    EXTI_ClearITPendingBit(EXTI_Line8); EXTI->IMR|=EXTI_Line8;
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn); NVIC_EnableIRQ(EXTI9_5_IRQn);
    GPIO_SetBits(BOARD_DHT_PORT,BOARD_DHT_PIN); start=board_millis();
    while(!dht.complete && !dht.failed && (uint32_t)(board_millis()-start)<10)
        vTaskDelay(pdMS_TO_TICKS(1));
    EXTI->IMR&=~EXTI_Line8;
    NVIC_DisableIRQ(EXTI9_5_IRQn);
    return dht_decoder_result(&dht,temperature,humidity);
}
bool board_adc_read(uint16_t *value)
{
    uint32_t started=board_millis(),sum=0; unsigned i;
    DMA_Cmd(DMA1_Channel1,DISABLE); DMA_ClearFlag(DMA1_FLAG_GL1);
    DMA_SetCurrDataCounter(DMA1_Channel1,32); DMA_Cmd(DMA1_Channel1,ENABLE);
    ADC_Cmd(ADC1,ENABLE); board_delay_us(3); ADC_SoftwareStartConvCmd(ADC1,ENABLE);
    while(DMA_GetFlagStatus(DMA1_FLAG_TC1)==RESET) {
        if(DMA_GetFlagStatus(DMA1_FLAG_TE1)!=RESET || (uint32_t)(board_millis()-started)>=20) { ADC_Cmd(ADC1,DISABLE); DMA_Cmd(DMA1_Channel1,DISABLE); return false; }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    ADC_Cmd(ADC1,DISABLE); DMA_Cmd(DMA1_Channel1,DISABLE);
    for(i=0;i<32;i++) sum+=adc_buffer[i];
    *value=(uint16_t)(sum/32); return true;
}
/* Called by serialized DMA/USART interrupts at the same preemption priority. */
static void uart_collect(void)
{
    unsigned end=(256u-DMA_GetCurrDataCounter(DMA1_Channel5))%256u;
    if(DMA_GetFlagStatus(DMA1_FLAG_HT5)!=RESET && DMA_GetFlagStatus(DMA1_FLAG_TC5)!=RESET) {
        /* Both boundaries accumulated before service: data ownership is lost.
         * Discard this DMA window and expose a stream-gap epoch to the parser. */
        rx_errors++; rx_tail=rx_head; dma_position=end;
        DMA_ClearFlag(DMA1_FLAG_HT5|DMA1_FLAG_TC5); return;
    }
    while(dma_position!=end) {
        unsigned next=(rx_head+1)%512;
        if(next==rx_tail) { rx_drops+=512; rx_tail=rx_head; }
        else { rx_ring[rx_head]=rx_dma[dma_position]; rx_head=next; }
        dma_position=(dma_position+1)%256;
    }
}
void DMA1_Channel5_IRQHandler(void)
{
    uart_collect();
    DMA_ClearITPendingBit(DMA1_IT_HT5|DMA1_IT_TC5);
}
void USART1_IRQHandler(void)
{
    uint32_t sr=USART1->SR;
    if(sr&(USART_SR_IDLE|USART_SR_ORE|USART_SR_FE|USART_SR_NE)) {
        volatile uint32_t discard=USART1->DR; (void)discard;
        if(sr&(USART_SR_ORE|USART_SR_FE|USART_SR_NE)) {
            rx_errors++; rx_tail=rx_head;
            dma_position=(256u-DMA_GetCurrDataCounter(DMA1_Channel5))%256u;
        } else uart_collect();
    }
}
bool board_uart_pop(uint8_t *b)
{
    bool have=false;
    taskENTER_CRITICAL();
    if(rx_head!=rx_tail) { *b=rx_ring[rx_tail]; rx_tail=(rx_tail+1)%512; have=true; }
    taskEXIT_CRITICAL(); return have;
}
uint32_t board_uart_drops(void) { return rx_drops; }
uint32_t board_uart_errors(void) { return rx_errors; }
bool board_uart_send(const uint8_t *p,size_t n)
{
    uint32_t start=board_millis();
    if(!n || n>sizeof tx_dma) return false;
    memcpy(tx_dma,p,n); DMA_Cmd(DMA1_Channel4,DISABLE); DMA_ClearFlag(DMA1_FLAG_GL4);
    DMA_SetCurrDataCounter(DMA1_Channel4,(uint16_t)n); DMA_Cmd(DMA1_Channel4,ENABLE);
    while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET) {
        if(DMA_GetFlagStatus(DMA1_FLAG_TE4)!=RESET || (uint32_t)(board_millis()-start)>=50) { DMA_Cmd(DMA1_Channel4,DISABLE); return false; }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    DMA_Cmd(DMA1_Channel4,DISABLE); return true;
}
uint8_t board_keys(void)
{
    return (uint8_t)((GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1)==0?1:0)|
                    (GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_12)==0?2:0));
}
void board_apply(KitchenOutput o)
{
    GPIO_WriteBit(GPIOC,GPIO_Pin_13,o.lamp?Bit_RESET:Bit_SET);
    GPIO_WriteBit(GPIOB,GPIO_Pin_15,o.buzzer?Bit_SET:Bit_RESET);
    TIM_SetCompare3(TIM3,o.ventilation?BOARD_SERVO_OPEN_US:BOARD_SERVO_CLOSED_US);
}
static bool flash_read(void *ctx,unsigned slot,unsigned off,uint8_t *p,size_t n)
{
    uint32_t address=slot?BOARD_CONFIG_B:BOARD_CONFIG_A; (void)ctx;
    if(slot>1 || off+n>1024) return false;
    memcpy(p,(const void *)(address+off),n); return true;
}
static bool flash_erase(void *ctx,unsigned slot)
{
    FLASH_Status status; (void)ctx; if(slot>1) return false;
    FLASH_Unlock(); FLASH_ClearFlag(FLASH_FLAG_EOP|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
    status=FLASH_ErasePage(slot?BOARD_CONFIG_B:BOARD_CONFIG_A); FLASH_Lock(); return status==FLASH_COMPLETE;
}
static bool flash_program(void *ctx,unsigned slot,unsigned off,const uint8_t *p,size_t n)
{
    size_t i; uint32_t address=slot?BOARD_CONFIG_B:BOARD_CONFIG_A; bool ok=true; (void)ctx;
    if(slot>1 || (off&1) || (n&1) || off+n>1024) return false;
    FLASH_Unlock();
    for(i=0;i<n;i+=2) if(FLASH_ProgramHalfWord(address+off+i,(uint16_t)(p[i]|(uint16_t)p[i+1]<<8))!=FLASH_COMPLETE) { ok=false; break; }
    FLASH_Lock(); return ok;
}
ConfigBackend board_config_backend(void)
{
    ConfigBackend b={NULL,flash_read,flash_erase,flash_program}; return b;
}
void board_watchdog_start(void)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); IWDG_SetPrescaler(IWDG_Prescaler_64);
    IWDG_SetReload(3124); IWDG_ReloadCounter(); IWDG_Enable();
}
void board_watchdog_feed(void) { IWDG_ReloadCounter(); }
uint32_t board_reset_flags(void) { return reset_flags; }
static volatile CrashRecord retained_crash __attribute__((section("RETENTION"),aligned(8)));
bool board_crash_read(CrashRecord *record) { return crash_read(&retained_crash,record); }
void board_crash_clear(void) { crash_clear(&retained_crash); }
static void capture(uint32_t reason,const uint32_t *stack,uint32_t exc,uint32_t mask)
{
    CrashContext c={0}; unsigned i;
    c.reason=reason; c.uptime_ms=wall_ms; c.exception_return=exc; c.task_mask=mask;
    if(stack && (uint32_t)stack>=0x20000000u && (uint32_t)stack<=0x20004fe0u && !((uint32_t)stack&3u))
        for(i=0;i<8;i++) c.stack[i]=stack[i];
    c.cfsr=SCB->CFSR; c.hfsr=SCB->HFSR; c.mmfar=SCB->MMFAR; c.bfar=SCB->BFAR;
    c.msp=__get_MSP(); c.psp=__get_PSP();
    crash_write(&retained_crash,&c);
    __DSB();
}
void board_capture_health(uint32_t stale_mask) { capture(0x20000u,NULL,0,stale_mask); }
static void halt(void)
{
    __disable_irq(); GPIO_ResetBits(GPIOB,GPIO_Pin_15); GPIO_SetBits(GPIOC,GPIO_Pin_13);
    TIM_Cmd(TIM3,DISABLE); for(;;) { } /* Watchdog resets if already enabled. */
}
void board_panic(uint32_t code) { __disable_irq(); capture(code,NULL,0,0); halt(); }
void board_fault_entry(uint32_t *stack,uint32_t exception_return)
{ __disable_irq(); capture(41,stack,exception_return,0); halt(); }
void board_init(void)
{
    TIM_TimeBaseInitTypeDef t; TIM_OCInitTypeDef oc; ADC_InitTypeDef adc;
    DMA_InitTypeDef d; USART_InitTypeDef u; NVIC_InitTypeDef irq; EXTI_InitTypeDef exti; uint16_t start;
    SystemCoreClockUpdate();
    CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk; CYCLE_COUNT=0; CYCLE_CTRL|=1u;
    reset_flags=RCC->CSR; RCC_ClearFlag(); NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO|RCC_APB2Periph_ADC1|RCC_APB2Periph_USART1,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3|RCC_APB1Periph_TIM4,ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
    gpio(GPIOC,GPIO_Pin_13,GPIO_Mode_Out_PP); GPIO_SetBits(GPIOC,GPIO_Pin_13);
    gpio(GPIOB,GPIO_Pin_15,GPIO_Mode_Out_PP); GPIO_ResetBits(GPIOB,GPIO_Pin_15);
    gpio(GPIOB,GPIO_Pin_1|GPIO_Pin_12,GPIO_Mode_IPU);
    gpio(BOARD_DHT_PORT,BOARD_DHT_PIN,GPIO_Mode_Out_OD); GPIO_SetBits(BOARD_DHT_PORT,BOARD_DHT_PIN);
    TIM_TimeBaseStructInit(&t); t.TIM_Prescaler=(uint16_t)(SystemCoreClock/1000000u-1);
    t.TIM_Period=65535; TIM_TimeBaseInit(TIM4,&t); TIM_Cmd(TIM4,ENABLE);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource8);
    exti.EXTI_Line=EXTI_Line8; exti.EXTI_Mode=EXTI_Mode_Interrupt;
    exti.EXTI_Trigger=EXTI_Trigger_Rising_Falling; exti.EXTI_LineCmd=ENABLE; EXTI_Init(&exti);
    EXTI->IMR&=~EXTI_Line8; /* SPL only programs edge selection when ENABLE. */
    irq.NVIC_IRQChannel=EXTI9_5_IRQn; irq.NVIC_IRQChannelPreemptionPriority=1;
    irq.NVIC_IRQChannelSubPriority=0; irq.NVIC_IRQChannelCmd=DISABLE; NVIC_Init(&irq);
    gpio(GPIOB,GPIO_Pin_0,GPIO_Mode_AF_PP); t.TIM_Period=19999; TIM_TimeBaseInit(TIM3,&t);
    TIM_OCStructInit(&oc); oc.TIM_OCMode=TIM_OCMode_PWM1; oc.TIM_OutputState=TIM_OutputState_Enable;
    oc.TIM_Pulse=BOARD_SERVO_CLOSED_US; TIM_OC3Init(TIM3,&oc); TIM_OC3PreloadConfig(TIM3,TIM_OCPreload_Enable); TIM_Cmd(TIM3,ENABLE);
    gpio(GPIOA,GPIO_Pin_1,GPIO_Mode_AIN); RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    ADC_StructInit(&adc); adc.ADC_ContinuousConvMode=ENABLE; ADC_Init(ADC1,&adc);
    ADC_RegularChannelConfig(ADC1,ADC_Channel_1,1,ADC_SampleTime_239Cycles5);
    ADC_DMACmd(ADC1,ENABLE); ADC_Cmd(ADC1,ENABLE); board_delay_us(10);
    ADC_ResetCalibration(ADC1); start=(uint16_t)TIM4->CNT;
    while(ADC_GetResetCalibrationStatus(ADC1)) if((uint16_t)((uint16_t)TIM4->CNT-start)>5000) board_panic(10);
    ADC_StartCalibration(ADC1); start=(uint16_t)TIM4->CNT;
    while(ADC_GetCalibrationStatus(ADC1)) if((uint16_t)((uint16_t)TIM4->CNT-start)>5000) board_panic(11);
    ADC_Cmd(ADC1,DISABLE);
    DMA_StructInit(&d); d.DMA_PeripheralBaseAddr=(uint32_t)&ADC1->DR; d.DMA_MemoryBaseAddr=(uint32_t)adc_buffer;
    d.DMA_BufferSize=32; d.DMA_MemoryInc=DMA_MemoryInc_Enable;
    d.DMA_PeripheralDataSize=DMA_PeripheralDataSize_HalfWord; d.DMA_MemoryDataSize=DMA_MemoryDataSize_HalfWord;
    DMA_Init(DMA1_Channel1,&d);
    gpio(GPIOA,GPIO_Pin_9,GPIO_Mode_AF_PP); gpio(GPIOA,GPIO_Pin_10,GPIO_Mode_IN_FLOATING);
    USART_StructInit(&u); u.USART_BaudRate=BOARD_UART_BAUD; USART_Init(USART1,&u);
    DMA_StructInit(&d); d.DMA_PeripheralBaseAddr=(uint32_t)&USART1->DR; d.DMA_MemoryBaseAddr=(uint32_t)rx_dma;
    d.DMA_BufferSize=256; d.DMA_MemoryInc=DMA_MemoryInc_Enable; d.DMA_Mode=DMA_Mode_Circular;
    d.DMA_Priority=DMA_Priority_High; DMA_Init(DMA1_Channel5,&d);
    DMA_ITConfig(DMA1_Channel5,DMA_IT_HT|DMA_IT_TC,ENABLE); DMA_Cmd(DMA1_Channel5,ENABLE);
    d.DMA_MemoryBaseAddr=(uint32_t)tx_dma; d.DMA_BufferSize=sizeof tx_dma;
    d.DMA_DIR=DMA_DIR_PeripheralDST; d.DMA_Mode=DMA_Mode_Normal; DMA_Init(DMA1_Channel4,&d);
    USART_DMACmd(USART1,USART_DMAReq_Rx|USART_DMAReq_Tx,ENABLE);
    USART_ITConfig(USART1,USART_IT_IDLE,ENABLE); USART_ITConfig(USART1,USART_IT_ERR,ENABLE);
    irq.NVIC_IRQChannel=DMA1_Channel5_IRQn; irq.NVIC_IRQChannelPreemptionPriority=5;
    irq.NVIC_IRQChannelSubPriority=0; irq.NVIC_IRQChannelCmd=ENABLE; NVIC_Init(&irq);
    irq.NVIC_IRQChannel=USART1_IRQn; NVIC_Init(&irq); USART_Cmd(USART1,ENABLE);
}
