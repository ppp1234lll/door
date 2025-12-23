#include "iwdg.h"
#include "appconfig.h"


void IWDG_Init(u8 prer,u16 rlr)
{
	#ifdef IWDG_ENABLE
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); //使能对IWDG->PR IWDG->RLR的写
	
	IWDG_SetPrescaler(prer); //设置IWDG分频系数

	IWDG_SetReload(rlr);   //设置IWDG装载值

	IWDG_ReloadCounter(); //reload
	
	IWDG_Enable();       //使能看门狗
	#endif
}

//喂独立看门狗
void IWDG_Feed(void)
{
	#ifdef IWDG_ENABLE
	IWDG_ReloadCounter();//reload
	#endif
}




