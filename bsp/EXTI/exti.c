#include "exti.h"
#include "led.h"
#include "key.h"
#include "delay.h"
#include "usart.h"

/*************************************************************************************
*@description：外部中断驱动文件
*@author：不甘心的咸鱼（tb43915564），其他平台--小红书—不搭（414192836）
*@version：V1.1
*@modify date：2025/8/27
*版权所有，请勿商用，违者必究。
*Copyright(C) 
*All rights reserved
***************************************************************************************/

/*
*@description：外部中断初始化，PB1/PB12
*@para：none
*@return：none
*/
void EXTIX_Init(void)
{
 
 	EXTI_InitTypeDef EXTI_InitStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;


  	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);				//使能复用功能时钟

	/*PB1,KEY0*/
  	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource1);
	EXTI_InitStructure.EXTI_Line=EXTI_Line1;	
  	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;			//下降沿触发
  	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  	EXTI_Init(&EXTI_InitStructure);	 								//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	/*PB12,KEY1*/
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource12); 
  	EXTI_InitStructure.EXTI_Line=EXTI_Line12;
  	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;			//下降沿触发
  	EXTI_Init(&EXTI_InitStructure);									//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器


  	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;			//使能按键KEY1所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;	//抢占优先级2， 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;			//子优先级0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure); 

		NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;			//使能按键KEY0所在的外部中断通道
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;	//抢占优先级2 
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;			//子优先级1 
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;					//使能外部中断通道
  	NVIC_Init(&NVIC_InitStructure);  	  							//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
 
}


 
