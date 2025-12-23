#include "includes.h"
#include "bsp.h"
#include "start.h"
#include "delay.h"

static void system_setup(void);
/************************************************************
*
* Function name	: main
* Description	: 主函数
* Parameter		: 
* Return		: 
*	
************************************************************/
int main(void)
{
	system_setup();
	start_system_init_function(); // 初始化系统
	OSInit(); 										// UCOS初始化
	start_creat_task_function();  // 初始化任务
	OSStart(); 					          // 开启UCOS
}

/************************************************************
*
* Function name	: 
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
static void system_setup(void)
{
	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0xE000);
	SystemInit();
	
	delay_init();		 							// 初始化延时函数
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	// 中断分组配置
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
}

