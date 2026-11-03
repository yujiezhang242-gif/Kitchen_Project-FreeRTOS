#include "beep.h"
#include "usart.h"	 
/*************************************************************************************
*@description：BEEP驱动文件
*@author：不甘心的咸鱼（tb43915564），其他平台--小红书—不搭（414192836）
*@version：V1.1
*@modify date：2025/8/27
*版权所有，请勿商用，违者必究。
*Copyright(C) 
*All rights reserved
***************************************************************************************/
 
 
/*
*@description：BEEP初始化
*@para：结构体变量、端口号、管口号
*@return：none
*/
void Beep_Init(led_d *yinjiao,gpioled port,u16 pin)
{
	if(port==GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 
	else if(port==GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	else if(port==GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	else if(port==GPIOD) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	else if(port==GPIOE) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	else if(port==GPIOF) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
	else if(port==GPIOG) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);

	
	yinjiao->port=port;
	yinjiao->pin=pin;
	
	Beep_config(yinjiao);
}
/*----------------------------------------------------------------------------------*/

/*
*@description：IO初始化
*@para：结构体变量
*@return：none
*/
void Beep_config(led_d *yinjiao)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = yinjiao->pin;				
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 
	GPIO_Init(yinjiao->port, &GPIO_InitStructure);					 
	Beep_off(yinjiao);
}
/*----------------------------------------------------------------------------------*/


 /*
*@description：关闭蜂鸣器
*@para：结构体变量
*@return：none
*/
void Beep_off(led_d *yinjiao)
{
	GPIO_ResetBits(yinjiao->port,yinjiao->pin);
}
/*----------------------------------------------------------------------------------*/


 /*
*@description：关闭蜂鸣器
*@para：结构体变量
*@return：none
*/
void Beep_on(led_d *yinjiao)
{
	GPIO_SetBits(yinjiao->port,yinjiao->pin);
}
/*----------------------------------------------------------------------------------*/
