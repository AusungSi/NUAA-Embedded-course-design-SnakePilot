/***********************************************************************
文件名称：Delay.C
功    能：定时器TIM1定时1秒，TIM2定时500ms，TIM3定时200ms，定时器到时分别使LED1、LED2和LED3改变状态（闪烁） 
编写时间：2017.3.29
编 写 人：
注    意：
***********************************************************************/
#include "stm32f10x.h"


void TIM_Configuration(void)
{

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
							 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);/*使能TIM1时钟，因为TIM1和TIM8连接在APB2上*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2|RCC_APB1Periph_TIM3, ENABLE);/*使能TIM2/TIM3时钟，因为TIM2~TIM7连接在APB1上*/

	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0	;/*不能少！对于TIM1/TIM8高级定时器必须把重新计数清零，否则TIM1定时器不准确，利用它可以重新不同的定时次数达到不同定时*/
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 10*1000-1; //1000ms
	TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/10000-1;//0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 10*500-1; //500ms
	TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/10000-1;//0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 10*200-1; //200ms
	TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/10000-1;//0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	
	
	/* TIM Interrupts enable */
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	
	/* TIM1/2/3 enable counter */
	TIM_Cmd(TIM1, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
}

/*********************************************
*函数名称：void Delayms(void)
*
*入口参数：N  ms
*
*出口参数：无
*
*功能说明：简单延时N ms
**********************************************/
void Delay_ms(uint16_t N)
{
	uint32_t i;
	for (i=0;i<(8000*N);i++);
}

