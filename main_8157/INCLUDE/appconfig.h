#ifndef _APPCONFIG_H_
#define _APPCONFIG_H_

/* 软件版本号 */
#define SOFT_NO_STR ("FN-ZJGX-PY-1.0.0.20250501") // 软件版本号
#define HARD_NO_STR	("FN-ZJGX-PY")			   	     // 硬件版本号

#define COM_DATA_VERSION  0x12  
//#define COM_DATA_VERSION  0x11

/* 代码开关位 */
#define DET_TOTALVAL_STATUS  	1   // 总电压安全检查功能
#define DET_TOTALVIN_STATUS  	1	 	// 总电流安全检查功能

/* 标志 - 屏蔽功能宏定义后，不启用功能 */
//#define COM_GPS_ENABLE   // GPS功能
//#define IWDG_ENABLE 		 	 // 看门狗功能
//#define SPD_ENABLE 		     // 防雷器
//#define BAT_ENABLE 		     // 电池检测
//#define SIM_ENABLE 		     // SIM卡检测
#define ERR_ENABLE 		     // 故障码使能

#define ANGLE_ENABLE       //倾斜度检测


#define PING_DEBUG   0
#define COM_DEBUG    0  
#define SAVE_DEBUG   0   // 保存
#define ONVIF_DEBUG  0
#define ERROR_DEBUG  0
#define DIRECT_REP_DEBUG  0   // 立刻上报
#define PRINT_DEBUG  0  // 打印调试
#define UPDATE_DEBUG  0  // 打印调试
#define GPRS_DEBUG  0  // 打印调试
#define ONVIF_SEARCH_DEBUG  0

#define THE_FIRST_TERM  1
#define THE_SECOND_TERM 2

#define VERSION_INFOR_FLAG THE_SECOND_TERM


#endif
