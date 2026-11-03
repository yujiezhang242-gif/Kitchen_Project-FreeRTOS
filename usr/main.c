/*************************************************************************************
*@description：main.c文件，应用层任务处理
*@author：不甘心的咸鱼（tb43915564），其他平台--小红书—不搭（414192836）
*@version：V1.1
*@modify date：2025/8/27
*版权所有，请勿商用，违者必究。
*Copyright(C) 
*All rights reserved
***************************************************************************************/

#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "usart.h"
#include "timer.h"
#include "FreeRTOS.h"
#include "adc.h"
#include "exti.h"
#include "lcd.h"
#include "gui.h"
#include "dht11.h"
#include "beep.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"


/*-----------------------------------------------------------FreeRTOS任务设置-------------------------------------------------------------------------------*/
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			1 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				64
//任务句柄
TaskHandle_t Start_Task_Handler;
//任务函数
static void 	 start_task(void *pvParameters);	
 			   
//LED任务
//设置任务优先级
#define LED0_TASK_PRIO       			5
//设置任务堆栈大小
#define LED0_STK_SIZE  		    		512
//任务句柄	
TaskHandle_t LED0_Task_Handler;
//任务函数
static void 	led0_task(void *pvParameters);


//智能窗任务
//设置任务优先级
#define PWM_TASK_PRIO       			4
//设置任务堆栈大小
#define PWM_STK_SIZE  					64
//任务句柄
TaskHandle_t PWM_Task_Handler;
//任务函数
static void 	pwm_task(void *pvParameters);

//光敏电阻任务
#define ADC_TASK_PRIO            		2	
#define ADC_STK_SIZE					512
TaskHandle_t ADC_Task_Handler;
static void 	adc_task(void *pvParameters);

//dht11任务
#define DHT11_TASK_PRIO 				3
#define DHT11_STK_SIZE  			  	512
TaskHandle_t DHT11_Task_Handler;
static void dht11_task(void *pvParameters);

//蜂鸣器开启任务
#define BEEP_TASK_PRIO         			6
#define BEEP_STK_SIZE         	 		64
TaskHandle_t BEEP_Task_Handler;
static void beep_task(void *pvParameters);
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*-----------------------------------------定义全局变量-------------------------------------------------------------------------------------------------------*/
static 	QueueHandle_t 		sig1;
static 	SemaphoreHandle_t 	sig2;
static 	SemaphoreHandle_t 	sig3;
led_d led1;
led_d p14;
led_d bep;
static u8 flag=0;
static u8 mark=0;
static u8 biao=0; 
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：主函数，初始化硬件
*@para：void
*@return：none
*/
 int main(void)
 {	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);	//设置中断分组
	delay_init();	    							//延时函数初始化		 
	LED_Init(&led1,GPIOC,GPIO_Pin_13);			 	//初始化与LED连接的硬件接口
	Beep_Init(&bep,GPIOB,GPIO_Pin_15);				//蜂鸣器初始化
	TIM3_PWM_Init(99,14399);						//舵机初始化
	Adc_Init();										//光敏传感器初始化
	EXTIX_Init();									//按键初始化
	LCD_Init();										//LCD初始化
	uart_init(9600);								//串口初始化
	xTaskCreate(start_task,"start_task",START_STK_SIZE,NULL,START_TASK_PRIO,&Start_Task_Handler);	//创建起始任务
	vTaskStartScheduler(); 							//开启多任务调度 	  	 
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：开始任务，创建其他任务
*@para：*pvParameters
*@return：none
*/
static void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();			//进入临界区(无法被中断打断) 
	
	/**********创建队列*******************/
	sig1=xQueueCreate(1,sizeof(u16));
	/*************************************/   
	
	/*****创建二值信号量--FreeRTOS********/
	sig2=xSemaphoreCreateBinary();
	sig3=xSemaphoreCreateBinary();
	/*************************************/ 
	
	/*创建任务*/
 	xTaskCreate(led0_task,"led0_task",LED0_STK_SIZE,NULL,LED0_TASK_PRIO,&LED0_Task_Handler);						   
 	xTaskCreate(pwm_task,"pwm_task",PWM_STK_SIZE,NULL,PWM_TASK_PRIO,&PWM_Task_Handler);	
	xTaskCreate(adc_task,"adc_task",ADC_STK_SIZE,NULL,ADC_TASK_PRIO,&ADC_Task_Handler);
	xTaskCreate(dht11_task,"dht11_task",DHT11_STK_SIZE,NULL,DHT11_TASK_PRIO,&DHT11_Task_Handler);
	xTaskCreate(beep_task,"beep_task",BEEP_STK_SIZE,NULL,BEEP_TASK_PRIO,&BEEP_Task_Handler);
	vTaskDelete(Start_Task_Handler); 		//删除开始任务
	taskEXIT_CRITICAL();					//退出临界区(可以被中断打断)
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：LED任务，接收光亮值，自动开灯
*@para：*pvParameters
*@return：none
*/
static void led0_task(void *pvParameters)
{	
	BaseType_t xResult;
	u16 light_num;
	while(1)
	{
		/* 从队列中获取数据，等待时间为portMAX_DELAY，一直等待 */
        xResult = xQueueReceive(sig1, &light_num, portMAX_DELAY);
		if(xResult == pdPASS){ 				/* 接收到消息 */
			led_on(&led1);
			Show_Str(0,20,RED,WHITE,"Light:00.00K",16,0);
			LCD_ShowNum(0+6*8,20,light_num/1000,2,16);
			LCD_ShowNum(0+9*8,20,light_num%1000,2,16);
		}
	}
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：PWM任务，湿度过高时，舵机转动
*@para：*pvParameters
*@return：none
*/
static void pwm_task(void *pvParameters)
{	
	while(1)
	{
		xSemaphoreTake(sig2,portMAX_DELAY);		/*等待信号量，portMAX_DELAY代表无限等*/
		biao+=1;								/*标志量*/
		TIM_SetCompare3(TIM3,13);				/*舵机转动*/
	}
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：光照采集任务
*@para：*pvParameters
*@return：none
*/
static void adc_task(void *pvParameters)
{
	
	u16 adcx;
	u16 RL;
	while(1){
		adcx=Get_Adc_Average(ADC_Channel_1,10);
		RL=(adcx*10000)/(4096-adcx);						/*计算光照强度，以电阻值表示，光强越大，电阻越小*/
		Show_Str(0,40,BLUE,WHITE,"Light:00.00K",16,0);
		LCD_ShowNum(0+6*8,40,RL/1000,2,16);
		LCD_ShowNum(0+9*8,40,RL%1000,2,16);
		if(RL>=10000) xQueueSend(sig1,&RL,portMAX_DELAY);	/*通过队列发送光照强度*/
		else{
			led_off(&led1);
			LCD_Fill(0,0,128,35,WHITE);
		} 
		delay_ms(1000);
	}
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：dht11任务
*@para：*pvParameters
*@return：none
*/
static void dht11_task(void *pvParameters)
{
		u8 temp;
		u8 humi;
		while(1)
		{
			DHT_Read_Data(&temp,&humi,GPIOC,GPIO_Pin_14,&p14);
			Show_Str(0,60,BLUE,WHITE,"Temp:00C",16,0);
			LCD_ShowNum(0+5*8,60,temp,2,16);
			Show_Str(0,80,BLUE,WHITE,"Humi:00RH",16,0);
			LCD_ShowNum(0+5*8,80,humi,2,16);
			printf("%d,%d",temp,humi);						/*发送温湿度信息到WIFI模块的串口，从而上传到APP*/
			if(temp>=30) xSemaphoreGive(sig3);				/*发送二值信号量*/
			else if(temp<=24)  TIM_SetCompare3(TIM3,3);		/*舵机转动*/
			if(humi>=70)									/*如果湿度过高*/
			{
				xSemaphoreGive(sig2);						/*发送二值信号量*/
				
				/*以下代码用于防止舵机根据温湿度的变化频繁开关*/
				if(biao==1)
				{
					biao=0;
					vTaskSuspend(PWM_Task_Handler);			/*挂起任务*/
				}
			}
			else if(humi<=50) vTaskResume(PWM_Task_Handler);
			delay_ms(1000);
		}
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：蜂鸣器报警任务
*@para：*pvParameters
*@return：none
*/
static void beep_task(void *pvParameters)
{
		while(1)
		{
			xSemaphoreTake(sig3,portMAX_DELAY);			   /*等待信号量*/
			Beep_on(&bep);								   /*开启蜂鸣器报警*/
		}
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：手动关闭蜂鸣器任务
*@para：none
*@return：none
*/
void EXTI1_IRQHandler(void)//外部中断1
{
		delay_xms(10);									/*延时消抖*/
		if(KEY0==0)										/*读取按键状态*/
			{
				flag+=1;								/*按下次数*/	
				if(flag%2==1)							/*第一次按下*/
				{
					Beep_off(&bep);						/*关闭蜂鸣器*/
					vTaskSuspend(BEEP_Task_Handler);	/*挂起蜂鸣器任务*/
				}
				else if(flag%2==0)						/*第二次按下*/
				{
					 vTaskResume(BEEP_Task_Handler);	/*恢复蜂鸣器任务*/
					 flag=0;							/*重置*/
				}
				Show_Str(0,100,BLUE,WHITE,"Flag:",16,0);
				LCD_ShowNum(0+5*8,100,flag,2,16);
			}
		EXTI_ClearITPendingBit(EXTI_Line1);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：在黑暗环境下强制手动关闭LED任务
*@para：none
*@return：none
*/
void EXTI15_10_IRQHandler(void)//外部中断PB12
{
	  delay_xms(10);
	  if(KEY1==0)										/*读取按键状态*/
	    {
				mark+=1;								/*按下次数*/
				if(mark%2==1)
				{
					led_off(&led1);
					vTaskSuspend(LED0_Task_Handler);
				}
				else if(mark%2==0)
				{
					 vTaskResume(LED0_Task_Handler);
					 mark=0;
				}
				Show_Str(0,120,BLUE,WHITE,"Mark:",16,0);
				LCD_ShowNum(0+5*8,120,mark,2,16);
	    }
		EXTI_ClearITPendingBit(EXTI_Line12);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


