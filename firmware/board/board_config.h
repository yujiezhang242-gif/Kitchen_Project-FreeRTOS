#ifndef KITCHEN_BOARD_CONFIG_H
#define KITCHEN_BOARD_CONFIG_H
/* F103C8, 8MHz crystal; system_stm32f10x.c selects 72MHz PLL.
 * OLED SSD1306-compatible I2C: PB6=SCL PB7=SDA, address 0x3C.
 * Analog light PA1; DHT11 PB8 (external pull-up required).
 * Servo TIM3_CH3 PB0; active-low LED PC13; active-high buzzer PB15.
 * Active-low keys PB1/PB12; serial PA9/PA10 115200 8N1.
 * GPIO and module polarity must be checked before connecting actuators. */
#define BOARD_OLED_ADDRESS 0x3cu
#define BOARD_DHT_PORT GPIOB
#define BOARD_DHT_PIN GPIO_Pin_8
#define BOARD_SERVO_CLOSED_US 1000u
#define BOARD_SERVO_OPEN_US 2000u
#define BOARD_UART_BAUD 115200u
#define BOARD_CONFIG_A 0x0800F800u
#define BOARD_CONFIG_B 0x0800FC00u
#define BOARD_FLASH_PAGE_BYTES 1024u
#endif
