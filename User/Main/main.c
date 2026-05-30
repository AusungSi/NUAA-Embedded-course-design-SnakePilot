/********************************************************
1.本实验硬件平台：NUAA_CM3_107实验开发板WEEEDK，MCU：STM32F107VCT6
               
2.开发和实验环境：MDK-ARM Keil V5.15及其以上版本
3.例程使用的STM32F10x固件库V3.5.0
4.本实验例程现象说明内容：  
	采用WEEEDK NUAA_CM3_107实验开发板
		TIMx定时器应用-更新方式定时实验
		初始化TIMx之后，练习指定时间器采用更新中断定时，在定时中断服务程序中让LED发光管闪烁。
	 
**************************************************************************************************/	

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "hw_config.h"
#include "stm32f10x.h"
#include "lcd.h"
//#include "delay.h"


int main(void)
{
	SystemInit();
	GPIO_Configuration();
	TIM_Configuration();         			/*  初始化TIMx定时器  */
	NVIC_Configuration();	
	LCD_Init();												/*  LCD初始化        */
	Welcome(); 										 		/*  显示主界面       */
	LED1(1);LED2(1);LED3(1);LED4(1);
	while(1)
	{		
		/*  本例程，主循环体不做事情，全部在中断函数中让LED闪烁，可根据需要自行加功能代码*/
	}
}


/***********************END OF FILE****/
