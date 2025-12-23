#include "start.h"
#include "includes.h"
#include "appconfig.h"
#include "app.h"
#include "delay.h"
#include "timer.h"
#include "key.h"
#include "adc.h"
#include "w25qxx.h"
#include "led.h"
#include "relay.h"
#include "eth.h"
#include "malloc.h"
#include "det.h"
#include "gsm.h"
#include "print.h"
#include "cJSON.h"
#include "hal_lis3dh.h"
#include "aht20.h"
#include "LAN8720.h"
#include "lwip_comm.h"
#include "httpd.h"
#include "lwip_ping.h"
#include "com.h"
#include "rtc.h"
#include "update.h"
#include "bootload.h"
#include "save.h"
#include "iwdg.h"
#include "rn8207c.h"
#include "rs485.h"
#include "usart2.h"
#include "lfs_port.h"
#include "adc.h"
#include "fan.h"

ChipID_t g_chipid_t;
code_id_t code_id_crc;

/* 初始化PVD */
void PVD_Init(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); //使能PVD电压检测模块的时钟
	PWR_PVDLevelConfig(PWR_PVDLevel_2V3); // 设定监控阀值
	PWR_PVDCmd(ENABLE); // 使能PVD

	EXTI_InitStructure.EXTI_Line = EXTI_Line16; // PVD连接到中断线16上
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt; //使用中断模式
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;//电压低于阀值时产生中断
	EXTI_InitStructure.EXTI_LineCmd = ENABLE; // 使能中断线
	EXTI_Init(&EXTI_InitStructure); // 初始
	
	NVIC_InitStructure.NVIC_IRQChannel = PVD_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/* PVD中断处理 */
void PVD_IRQHandler(void)
{
	EXTI_ClearITPendingBit(EXTI_Line16);
	if(PWR_GetFlagStatus( PWR_FLAG_PVDO ))    /* 1为VDD小于PVD阈值,掉电情况 */
	{
		while(1)
		{
			printf("reset:0x%8x\r\n",RCC->CSR);
			for(uint32_t i=0;i<0x50000;i++);
			IWDG_Feed();
			pwr_tst_detection();
			if(det_get_220v_in_function() == 1)  // 220V上电
				System_SoftReset();			
		}
	}
}
/************************************************************
*
* Function name	: start_system_init_function
* Description	: 系统初始化函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void start_system_init_function(void)
{
	cJSON_Hooks hook;
	PVD_Init();
	IWDG_Init(IWDG_Prescaler_64,1000);				// 初始化看门狗 2s
	
	start_get_device_id_function();					// 获取本机ID
	mymem_init(SRAMIN);											// 内存初始化
	
	hook.malloc_fn = mymalloc_sramin;
	hook.free_fn   = myfree_sramin;
	cJSON_InitHooks(&hook);
	
	led_gpio_init_function();						// LED初始化（已测试）
	relay_gpio_init_function();				  // 继电器初始化（已测试）
	
	fan_gpio_init_function();
	
//	relay_test();
	
	usart2_init_function(115200);       // 调试接口——串口2（已测试）
	key_init_function();	 						  // 按键初始化(已测试)
	RTC_Init();													// RTC初始化(已测试)

	rn8207c_init_function();						// 电能检测初始化(已测试)
		
	IWDG_Feed();

	hal_lis3dh_init(false);	 						// 陀螺仪初始化(未测试)
	
//	lis3dh_test();
	
	aht20_init_function();							// 温湿度初始化(未测试)

	printf("ceshi\r\n");
	printf("RCC->CSR2..:0x%08x..\r\n",RCC->CSR);	
	RCC_ClearFlag();
		
	while((RCC->CSR & 0x08000000) != 0)
	{ 
		pwr_tst_detection();
		if(det_get_220v_in_function() == 1)  // 220V上电
		{
			RCC_ClearFlag();
			System_SoftReset();
		}
		else
		{
//			printf("POR RESET\r\n");
			delay_ms(1000);
			IWDG_Feed();
		}
	}
	RCC_ClearFlag();                // 清楚标志位
	IWDG_Feed();
	W25QXX_Init();			 						// 初始化spiflash
	save_init_function();	
	com_recevie_function_init();					// 初始化接收缓冲区
	app_get_storage_param_function();				// 获取本地存储的数据
//	my_modem_init();		// 更新检测
	
	TIM3_Int_Init(1000-1,72-1);					// 定时器3初始化(已测试)
	TIM2_Int_Init(10000-1,72-1);				// 定时器2初始化(已测试)
	TIM6_Int_Init(10000-1,7200-1);		  // 初始化定时器6   1Hz
	
	IWDG_Feed();
}


/************************************************************
*
* Function name	: start_get_device_id_function
* Description	: 获取本机ID
* Parameter		: 
* Return		: 
*	
************************************************************/
void start_get_device_id_function(void)
{
	volatile uint32_t addr;
	addr  = 0x20000006;
	addr -= 0x800;
	addr -= 0x1e;	
	
	g_chipid_t.id[0] = *(__I uint32_t *)(addr + 0x00);
	g_chipid_t.id[1] = *(__I uint32_t *)(addr + 0x04);
	g_chipid_t.id[2] = *(__I uint32_t *)(addr + 0x08);
}

/************************************************************
*
* Function name	: start_get_device_id_str
* Description	: 获取本机ID
* Parameter		: 
* Return		: 
*	
************************************************************/
void start_get_device_id_str(uint8_t *str)
{
	sprintf((char*)str,"%04X%04X%04X",g_chipid_t.id[0],g_chipid_t.id[1],g_chipid_t.id[2]);
}

void start_get_device_id(uint32_t *id)
{
	id[0] = g_chipid_t.id[0];
	id[1] = g_chipid_t.id[1];
	id[2] = g_chipid_t.id[2];
}

/* 任务优先级 */
#define APP_TASK_PRIO		8
/* 任务堆栈大小 */
#define APP_STK_SIZE		400
/* 任务堆栈 */
OS_STK START_TASK_STK[APP_STK_SIZE];
/* 任务函数 */
void app_task(void *argument);

/* 任务优先级 */
#define ETH_TASK_PRIO		10
/* 任务堆栈大小 */
#define ETH_STK_SIZE		320
/* 任务堆栈 */
OS_STK ETH_TASK_STK[ETH_STK_SIZE];
/* 任务函数 */
void eth_task(void *argument);

/* 任务优先级 */
#define DET_TASK_PRIO		6
/* 任务堆栈大小 */
#define DET_STK_SIZE		256
/* 任务堆栈 */
OS_STK DET_TASK_STK[DET_STK_SIZE];
/* 任务函数 */
void det_task(void *argument);

/* 任务优先级 */
#define GSM_TASK_PRIO		9
/* 任务堆栈大小 */
#define GSM_STK_SIZE		320
/* 任务堆栈 */
OS_STK GSM_TASK_STK[GSM_STK_SIZE];
/* 任务函数 */
void gsm_task(void *argument);


#define PRINT_TASK_PRIO		15   /* 任务优先级 */
#define PRINT_STK_SIZE		256 /* 任务堆栈大小 */
OS_STK 	PRINT_TASK_STK[PRINT_STK_SIZE]; /* 任务堆栈 */
void print_task(void *argument); /* 任务函数 */
/************************************************************
*
* Function name	: start_creat_task_function
* Description	: 创建任务
* Parameter		: 
* Return		: 
*	
************************************************************/
void start_creat_task_function(void)
{
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();  																	// 关中断
	
	/* 创建任务 */
	OSTaskCreate(app_task,(void*)0,(OS_STK*)&START_TASK_STK[APP_STK_SIZE-1],APP_TASK_PRIO); 	// 创建主要任务
	OSTaskCreate(eth_task,(void*)0,(OS_STK*)&ETH_TASK_STK[ETH_STK_SIZE-1],ETH_TASK_PRIO);		// 创建网口检测任务 - 低优先级
	OSTaskCreate(det_task,(void*)0,(OS_STK*)&DET_TASK_STK[DET_STK_SIZE-1],DET_TASK_PRIO);   	// 创建检测任务
	OSTaskCreate(gsm_task,(void*)0,(OS_STK*)&GSM_TASK_STK[GSM_STK_SIZE-1],GSM_TASK_PRIO);		// 无线网络任务
	OSTaskCreate(print_task,(void*)0,(OS_STK*)&PRINT_TASK_STK[PRINT_STK_SIZE-1],PRINT_TASK_PRIO);		// 打印搜索任务
	OS_EXIT_CRITICAL();  		 															// 开中断
}

/************************************************************
*
* Function name	: app_task_function
* Description	: 主要任务函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void app_task(void *argument)
{
	OSStatInit();	  	// 初始化统计任务
	app_task_function();
}


/************************************************************
*
* Function name	: eth_task
* Description	: 网口检测任务:一直对网口进行轮询
* Parameter		: 
* Return		: 
*	
************************************************************/
void eth_task(void *argument)
{
	eth_network_line_status_detection_function();
}

/************************************************************
*
* Function name	: det_task
* Description	: 检测函数：包括温湿度、adc等
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_task(void *argument)
{
	det_task_function();
}

/************************************************************
*
* Function name	: gsm_task
* Description	: 2G模块任务：打电话
* Parameter		: 
* Return		: 
*	
************************************************************/
void gsm_task(void *argument)
{
	gsm_task_function();
}

/************************************************************
*
* Function name	: gsm_task
* Description	: 2G模块任务：打电话
* Parameter		: 
* Return		: 
*	
************************************************************/
void print_task(void *argument)
{
	print_task_function();
}	


/************************************************************
*
* Function name	: gsm_task
* Description	: 2G模块任务：打电话
* Parameter		: 
* Return		: 
*	
************************************************************/
void task_suspend_function(void)
{
	OSTaskSuspend(APP_TASK_PRIO);
	OSTaskSuspend(ETH_TASK_PRIO);
	OSTaskSuspend(DET_TASK_PRIO);
	OSTaskSuspend(PRINT_TASK_PRIO);
}	




