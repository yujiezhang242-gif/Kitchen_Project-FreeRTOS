#ifndef KITCHEN_BOARD_H
#define KITCHEN_BOARD_H
#include "kitchen.h"
#include "config_store.h"
#include "crash.h"
#include <stddef.h>
/* Default kit wiring, to be confirmed against actual modules. */
void board_init(void);
uint32_t board_millis(void);
uint32_t board_cycles(void);
uint32_t board_cycles_per_ms(void);
void board_delay_us(uint16_t us);
bool board_adc_read(uint16_t *value);
bool board_dht_read(int16_t *temperature,uint16_t *humidity);
uint8_t board_keys(void);
void board_apply(KitchenOutput output);
bool board_display_init(void);
bool board_display_line(unsigned row,const char *text);
bool board_uart_pop(uint8_t *byte);
bool board_uart_send(const uint8_t *data,size_t size);
uint32_t board_uart_drops(void);
uint32_t board_uart_errors(void);
ConfigBackend board_config_backend(void);
void board_watchdog_start(void);
void board_watchdog_feed(void);
uint32_t board_reset_flags(void);
void board_panic(uint32_t code);
void board_capture_health(uint32_t stale_mask);
bool board_crash_read(CrashRecord *record);
void board_crash_clear(void);
void board_fault_entry(uint32_t *stack,uint32_t exception_return);
#endif
