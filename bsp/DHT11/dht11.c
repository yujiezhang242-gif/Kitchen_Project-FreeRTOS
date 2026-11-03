#include "dht11.h"
#include "led.h"
/*************************************************************************************
*@description：DHT11驱动文件
*@author：不甘心的咸鱼（tb43915564），其他平台--小红书—不搭（414192836）
*@version：V1.1
*@modify date：2025/8/27
*版权所有，请勿商用，违者必究。
*Copyright(C) 
*All rights reserved
***************************************************************************************/
u8  BUF[5]={0x00,0x00,0x00,0x00,0x00};    //存储读取的温湿度信息
u32 sum=0;         //校验值


/*
*@description：DHT11初始化
*@para：&io:IO口
*@return：none
*/
void DHT11_Start(led_d *io)  //DHT11数据输入引脚PC14
{
	
	GPIO_InitTypeDef GPIO_InitStructure;
	//RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE); 
	clock_init(io);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 			//推挽输出模式
	GPIO_InitStructure.GPIO_Pin = io->pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(io->port, &GPIO_InitStructure);
	GPIO_ResetBits(io->port,io->pin); 							//拉低数据线
	delay_ms(20);    											//拉低至少18ms
	GPIO_SetBits(io->port,io->pin); 							//拉高数据线 
	delay_us(30);     											//主机拉高20~40us
	GPIO_ResetBits(io->port,io->pin);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/


/*
*@description：DHT11初始化
*@para：&io:IO口
*@return：none
*/
void DHT11_Read(led_d *io)

{
	GPIO_InitTypeDef GPIO_InitStructure;
	clock_init(io);
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; //推挽输出模式
	GPIO_InitStructure.GPIO_Pin = io->pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(io->port, &GPIO_InitStructure);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/

/*
*@description：DHT11读一个字节
*@para：*io，结构体变量
*@return：ReadDat
*/
u8 DHT_Read_Byte(led_d *io)
{
	u8 temp;  
	u8 ReadDat=0; 
	u8 t = 0;
	u8 i; 
 
	for(i=0;i<8;i++)
	{
		while(readpin(io)==0&&t<100)  
		{		
			delay_us(1);
			t++;  //防止卡死
		}
		t=0;
		//由于‘0’代码高电平时间26~28us，1代码高电平时间70us，延时30us，可判断高低电平，数字0读取到的是低电平，高电平则反之
		delay_us(30);
		temp=0;
		if(readpin(io)==1) temp=1;		
			
		while(readpin(io)==1&&t<100)
		{		
			delay_us(1);
			t++;
		}
		t=0;
		ReadDat<<=1; //左移1位，DHT11数据由高到低发送
		ReadDat|=temp;
	}	
	return ReadDat;
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/ 


/*
*@description：DHT11读数据
*@para：*temp：温度
*@para：*humi：湿度
*@para：port：GPIO端口
*@para：pin：管脚号
*@para：*io：结构体变量
*@return：1，成功；0
*/
u8 DHT_Read_Data(u8 *temp,u8 *humi,gpioled port,u16 pin,led_d *io)
{
	u8 i;
	u8 t = 0;
	io->port=port;
	io->pin=pin;
	DHT11_Start(io);
	DHT11_Read(io);
	delay_us(20);
	//延时20us，低电平80us，还剩60us，检查是否是低电平以确定是否有响应信号
	if(readpin(io)==0)  			//如果读取到低电平，证明DHT11响应
	{
		while(readpin(io)==0&&t<100)//接收响应信号低电平剩余60us，等待变高电平
		{
			delay_us(1);
			t++;			
		}
		t=0;						//超过100us自动向下运行，以免卡死
		while(readpin(io)==1&&t<100)//接收响应信号高电平80us，等待变低电平
			{
				delay_us(1);
				t++;			
			}
		t=0;
		for(i=0;i<5;i++) 		 	//接收40位数据
			{
				BUF[i]=DHT_Read_Byte(io);//读出1个字节
			}
		delay_us(50);					 //结束信号
	}
	sum=BUF[0]+BUF[1]+BUF[2]+BUF[3];	 // BUF[0]：湿度整数值 BUF[1]：湿度小数值 BUF[2]：温度整数值 BUF[3]：温度小数值 BUF[4]：校验位，都为8位数据
	if(BUF[4]==(u8)sum)  				//校验  
	{
		*humi=BUF[0];
		*temp=BUF[2];
		return 1;    
	}
	else
		return 0;    
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/ 


/*
*@description：时钟初始化
*@para：*io，结构体变量
*@return：none
*/
void clock_init(led_d *io)
{
		if(io->port==GPIOA) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 
		else if(io->port==GPIOB) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		else if(io->port==GPIOC) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		else if(io->port==GPIOD) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
		else if(io->port==GPIOE) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
		else if(io->port==GPIOF) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOF, ENABLE);
		else if(io->port==GPIOG) RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);

}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/ 



/*
*@description：读GPIO口状态
*@para：*io，结构体变量
*@return：GPIO口状态
*/
u8 readpin(led_d *io)
{
	return GPIO_ReadInputDataBit(io->port,io->pin);
}
/*-----------------------------------------------------------------------------------------------------------------------------------------------------------*/ 


