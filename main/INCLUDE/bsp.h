#ifndef _BSP_H_
#define _BSP_H_
#include "sys.h"
#include "stdio.h"
#include "string.h"
#include "appconfig.h"

#define TIM_DELAY_TIME        (1000) // 定时器时间 单位us

#define PRIORITY_CONNECTION_MODE 1 // 0-有线优先连接 1-无线优先连接

#if (PRIORITY_CONNECTION_MODE == 1) 
#define WIRELESS_PRIORITY_CONNECTION // 无线优先连接
#else
#define WIRED_PRIORITY_CONNECTION    // 有线优先连接
#endif

/* Communication data processing mode */
#define COMDATA_PROCESS_MODE  (2) // 1-模式1（2020） 2-模式2（20210121）

#if (COMDATA_PROCESS_MODE == 1) 
#define COMDATA_PROCESS_MODE1 // 模式1
#else 
#define COMDATA_PROCESS_MODE2 // 模式2
#endif


#endif

