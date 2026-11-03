#ifndef __BEEP_H
#define __BEEP_H	 
#include "sys.h"
#include "led.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ���������ɣ��������������κ���;
//ALIENTEKս��STM32������
//LED��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/2
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) �������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 
//#define LED PCout(13)// PB5



void Beep_Init(led_d *yinjiao,gpioled port,u16 pin);//��ʼ��
void Beep_config(led_d *yinjiao);
void Beep_off(led_d *yinjiao);
void Beep_on(led_d *yinjioa);
#endif
