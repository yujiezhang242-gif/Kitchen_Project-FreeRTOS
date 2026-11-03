#include "led.h"
#include "usart.h"	 
/*************************************************************************************
*@description：LED驱动文件
*@author：不甘心的咸鱼（tb43915564），其他平台--小红书—不搭（414192836）
*@version：V1.1
*@modify date：2025/8/27
*版权所有，请勿商用，违者必究。
*Copyright(C) 
*All rights reserved
***************************************************************************************/

/*
*@description：LED初始化
*@para：*led：结构体变量
*@para：port：GPIO口
*@para：pin：管脚号
*@return：none
*/
void LED_Init(led_d *led,gpioled port,uint16_t pin)
{
	if(port==GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 
	else if(port==GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	else if(port==GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	else if(port==GPIOD) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	else if(port==GPIOE) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	else if(port==GPIOF) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	else if(port==GPIOG) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);

	
	led->port=port;
	led->pin=pin;
	
	led_config(led);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*
*@description：GPIO口初始化
*@para：*led：结构体变量
*@return：none
*/
void led_config(led_d *led)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = led->pin;				 		//端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 	//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 	//IO口速度为50MHz
	GPIO_Init(led->port, &GPIO_InitStructure);					//根据设定参数初始化
	led_off(led);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：关闭LED
*@para：*led：结构体变量
*@return：none
*/
void led_off(led_d *led)
{
	GPIO_SetBits(led->port,led->pin);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/



/*
*@description：打开LED
*@para：*led：结构体变量
*@return：none
*/
void led_on(led_d *led)
{
	GPIO_ResetBits(led->port,led->pin);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/
