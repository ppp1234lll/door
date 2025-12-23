#ifndef _DET_H_
#define _DET_H_
#include "bsp.h"

#define DET_ADAPTER_DATA (12)

typedef enum
{
	DO_CURRENT1 = 0, // 电流采集1
	DO_CURRENT2 = 1, // 电流采集2
	DO_CURRENT3 = 2, // 电流采集3
	DO_CURRENT4 = 3, // 电流采集4
	DO_LIGHT  	= 4, // 外部补光灯
} DET_OUTSIDE;    	 // 外部采集数据

typedef enum
{
	DW_LIGHT = 0, // 补光灯工作状态
	
} DET_WORK_STATUS;

typedef struct {
	int32_t AcceX;				/*acceleration x*/
	int32_t AcceY;				/*acceleration y*/
	int32_t AcceZ;				/*acceleration z*/
} MENS_XYZ_STATUS_T;


void det_task_function(void);

void det_get_key_status_function(void);
uint8_t det_main_network_and_camera_network(void);
void det_get_temphumi_function(void);
float lean_check(MENS_XYZ_STATUS_T *acc_xyz);
void det_get_attitude_state_value(void);
fp32 det_get_inside_temp(void);								// 获取内部温度
fp32 det_get_inside_humi(void); 							// 获取内部湿度
fp32 det_get_vin220v_handler(uint8_t num);					// 获取220v采集数据


void det_set_open_door(uint8_t mode);  // 设置箱门状态
void det_set_220v_in_function(uint8_t status); 			    // 设置市电状态
void det_set_camera_status(uint8_t num,uint8_t status);		// 设置摄像机状态
void det_set_main_network_status(uint8_t status);			// 设置主网络状态
void det_set_main_network_sub_status(uint8_t status);		// 设置主网络状态 - 2
void det_set_total_voltage(float data);						// 设置总电压
void det_set_total_current(float data);						// 设置总电流
void det_set_total_current_b(float data);          // 设置电流值(外置电流互感器)
void det_set_ping_status(uint8_t status);
void det_set_spd_status(uint8_t mode);   //  设置防雷开关状态


uint8_t det_get_open_door(void);							// 获取箱门状态
uint16_t det_get_cabinet_posture(void);						// 获取箱体姿态
uint8_t det_get_220v_in_function(void);						// 获取市电状态参数
uint8_t det_get_camera_status(uint8_t num);					// 获取摄像机状态
uint8_t det_get_main_network_status(void);					// 获取主网络状态
uint8_t det_get_main_network_sub_status(void);				// 获取主网络状态 - 2
uint8_t det_get_spd_status(void);	   //  获取防雷开关状态


#endif
