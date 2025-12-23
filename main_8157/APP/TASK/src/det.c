#include "det.h"
#include "includes.h"
#include "math.h"
#include "key.h"
#include "led.h"
#include "gsm.h"
#include "app.h"
#include "appconfig.h"
#include "com.h"
#include "eth.h"
#include "aht20.h"
#include "adc.h"
#include "hal_lis3dh.h"
#include "adc.h"
#include "key.h"
#include "iwdg.h"
#include "RN8207C.h"
#include "bsp.h"
#include "w25qxx.h"
#include "stmflash.h"
#include "save.h"

typedef struct
{
	float temp_inside; 	 // 内部温度值
	float humi_inside; 	 // 内部湿度值
	
	double attitude_acc; // 加速度

	float vin220v;		 // 220V电压检测
	float current_sm;		 // 总电流
	float total_current; // 第一路电流
	float total_current_b; //  第二路电流
	
	uint8_t open_door;	 // 开门检测
	uint8_t vinin;		 // 市电输入监测
	uint8_t spd;	 // 防雷检测
	
	uint8_t camera[10];	 // 摄像机状态x3：0；离线 1：在线，2：延时严重
	uint8_t main_ip;	 // 主网络状态：0：离线 1：在线，2：延时严重
	uint8_t main_sub_ip; // 主网络状态sub: 0：离线 1：在线，2：延时严重
	uint8_t ping_status;     // ping结束标志位
} data_collection_t;


data_collection_t sg_datacollec_t;

/************************************************************
*
* Function name	: det_task_function
* Description	: 检测任务：包括adc采集、开关量数据等
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_task_function(void)
{
#ifdef BAT_ENABLE
	Adc_Init();
#endif
	while(1)
	{
		key_detection_function();       // 按键循环查询
		det_get_key_status_function();	// 按键检测函数
		det_get_temphumi_function(); 		// 获取温湿度
		det_get_attitude_state_value(); // 获取姿态数据
		open_door_detection();			    // 检测箱门
		pwr_tst_detection();						// 检测12v供电
		rn8207_work_process_function();	// 数据获取函数
		spd_status_detection();   			// 防雷检测  20230721

		IWDG_Feed();					// 喂狗			
		OSTimeDlyHMSM(0,0,0,20);  	 	// 延时20ms

	}
}

/************************************************************
*
* Function name	: det_get_key_status_function
* Description	: 获取按键状态值
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_get_key_status_function(void)
{
	uint8_t key = 0;
	static uint16_t key_double_count = 0; // 双击计数
	static uint16_t key_double_time = 0; // 双击计数
	key = key_get_state_function(RESET_KEY_NUM);
	if(key_double_time > 0)
	{
		key_double_time--;
		if(key_double_time == 0)
			key = 0;
	}
	
	switch(key) // 检测到按下复位按键
	{
		case KEY_1_LONG:
			led_control_function(LD_STATE,LD_ON); 
			app_set_reset_function();/* 将所有设置参数设置为默认值，切网络需要重新连接 */
//			OSTimeDlyHMSM(0,0,2,0);  			// 延时10ms
//			System_SoftReset();  // 重启
		break;
		
		case KEY_1_TIEBLE:  // 三击
			key_double_count ++;
			key_double_time = 100;  // 判断2s内不操作退出
			if(key_double_count >= 4)
			{
				IWDG_Init(IWDG_Prescaler_256,3906); // 初始化看门狗 25s
				IWDG_Feed();
				key_double_count =0;
				key_double_time = 0;
				led_control_function(LD_STATE,LD_ON); 
				W25QXX_Erase_Chip();
				STMFLASH_ErasePage(DEVICE_MAC_ADDR);
				OSTimeDlyHMSM(0,0,2,0);  			// 延时10ms
				System_SoftReset();  // 重启
			}
		break;
		
		default: break;
	}
}

/************************************************************
*
* Function name	: det_main_network_and_camera_network
* Description	: 主网络与摄像头网络状态检查
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t det_main_network_and_camera_network(void)
{
	static uint8_t main_ip[2] = {0};
	static uint8_t camera[6]  = {0};
	uint8_t i = 0;
	
	/* 检测主网络 */
	if(sg_datacollec_t.main_ip == 0 && sg_datacollec_t.main_sub_ip == 0) 
	{
		if(main_ip[0] == 1 || main_ip[1] == 1) {
			main_ip[0] = sg_datacollec_t.main_ip;
			main_ip[1] = sg_datacollec_t.main_sub_ip;
			return 1;
		}
	}
	main_ip[0] = sg_datacollec_t.main_ip;
	main_ip[1] = sg_datacollec_t.main_sub_ip;
	
	/* 检测摄像头 */
	for(i=0; i<6; i++) 
	{
		if(camera[i] == 1 && sg_datacollec_t.camera[i] == 0) 
		{
			camera[0] = sg_datacollec_t.camera[0];
			camera[1] = sg_datacollec_t.camera[1];
			camera[2] = sg_datacollec_t.camera[2];
			camera[3] = sg_datacollec_t.camera[3];
			camera[4] = sg_datacollec_t.camera[4];
			camera[5] = sg_datacollec_t.camera[5];
			return 1;
		}
	}
	camera[0] = sg_datacollec_t.camera[0];
	camera[1] = sg_datacollec_t.camera[1];
	camera[2] = sg_datacollec_t.camera[2];
	camera[3] = sg_datacollec_t.camera[3];
	camera[4] = sg_datacollec_t.camera[4];
	camera[5] = sg_datacollec_t.camera[5];
	
	return 0;
}

/************************************************************
*
* Function name	: det_get_temphumi_function
* Description	: 检测温湿度
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_get_temphumi_function(void)
{
	double det_temp = 0;
	double det_humi = 0;
	
	if(aht20_measure(&det_humi,&det_temp) == 0) {
		sg_datacollec_t.humi_inside = det_humi;
		sg_datacollec_t.temp_inside = det_temp;
	}
}

float lean_check(MENS_XYZ_STATUS_T *acc_xyz)
{
	float ax_ref = 0;
	float ay_ref = 0;
	float az_ref = 1;
	float norm_ref = 1;
	
	double temp = 0;
	double norm = 0;
	double dot_norm = 0;
//	bool Ret = false;
	float ax,ay,az,dot; 
	MENS_XYZ_STATUS_T tmp;
	static MENS_XYZ_STATUS_T acc;
	float angle;
	
	#if 1
	/*去除加速度计抖动,30mg变化视为抖动*/	
	if(abs(acc_xyz->AcceX-acc.AcceX)>=30){
		tmp.AcceX = acc_xyz->AcceX;
		acc.AcceX = acc_xyz->AcceX;
	}else{
		tmp.AcceX = acc.AcceX;
	}
	if(abs(acc_xyz->AcceY-acc.AcceY)>=30){
		tmp.AcceY = acc_xyz->AcceY;
		acc.AcceY = acc_xyz->AcceY;
	}else{
		tmp.AcceY = acc.AcceY;
	}

	if(abs(acc_xyz->AcceZ-acc.AcceZ)>=30){
		tmp.AcceZ = acc_xyz->AcceZ;
		acc.AcceZ = acc_xyz->AcceZ;
	}else{
		tmp.AcceZ = acc.AcceZ;
	}
	#endif

	/*归一化处理*/
	temp = tmp.AcceX*tmp.AcceX + tmp.AcceY*tmp.AcceY + tmp.AcceZ*tmp.AcceZ;
	norm = (float)sqrt(temp);
	if (norm == 0.0) norm = 0.000001;  //avoid Nan happen
	ax = tmp.AcceX / norm;
	ay = tmp.AcceY / norm;
	az = tmp.AcceZ / norm;

	/*检测倾角是否超限40°*/

	/*ax_ref,ay_ref,az_ref
	为归一化后的参考向量的分量，可以自己设置，如定义为ax_ref=0,ay_ref=0,az_ref=1，表示加速度计正立放置
	也可以让终端程序稳定运行一段时间后，取一个加速度值作为参考向量，这样就可以不论物体怎么摆放，当他稳定后就可以确定初始向量*/
	dot = ax*ax_ref+ay*ay_ref+az*az_ref;
	
	dot_norm = (float)sqrt((double)(ax*ax) + (double)(ay*ay) + (double)(az*az));
	if (dot_norm == 0.0) dot_norm = 0.000001;
	//针对不少人的疑问:norm_ref和dot_norm 一样是归一化后的向量的模，norm_ref是参考的向量而已,可以在开机时记录，或者在某个特定时刻指定,计算公式参考dot_norm ,需要注意的是参考向量也需要归一化
	angle = acos(dot/(dot_norm*norm_ref))*180/3.14;//弧度转化为角度，
	
	return angle;
}

/************************************************************
*
* Function name	: det_get_attitude_state_value
* Description	: 检测陀螺仪姿态值
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_get_attitude_state_value(void)
{
#ifdef ANGLE_ENABLE
	MENS_XYZ_STATUS_T mens_t;
	short x = 0;
	short y = 0;
	short z = 0;
	
	hal_lis3dh_get_xyz(&x, &y, &z);
	
	/* 计算姿态值，并且判断箱体状态 */
	mens_t.AcceX = x;
	mens_t.AcceY = y;
	mens_t.AcceZ = z;
	sg_datacollec_t.attitude_acc = lean_check(&mens_t);

#endif
}

#define MODE_DET_PARAM 1
/************************************************************
*
* Function name	: det_get_inside_temp
* Description	: 检测内部温度
* Parameter		: 
* Return		: 
*	
************************************************************/
fp32 det_get_inside_temp(void)
{
	return (sg_datacollec_t.temp_inside);
}

/************************************************************
*
* Function name	: det_get_inside_humi
* Description	: 检测内部湿度
* Parameter		: 
* Return		: 
*	
************************************************************/
fp32 det_get_inside_humi(void)
{
	return (sg_datacollec_t.humi_inside);
}

/************************************************************
*
* Function name	: det_get_vin220v_handler
* Description	: 获取220V电压输入参数:电压、电流
* Parameter		: 
*	@num		: 0:220V电压 1:220电流
* Return		: 
*	
************************************************************/
fp32 det_get_vin220v_handler(uint8_t num)
{
	if(num == 0)
	{
		return sg_datacollec_t.vin220v;
	}
	else	if(num == 1)
	{
		return sg_datacollec_t.total_current;
	}
	else	if(num == 2)
	{
		return sg_datacollec_t.total_current_b;
	}
	else	if(num == 3)
	{
		return (sg_datacollec_t.total_current+sg_datacollec_t.total_current_b);
	}
	return  0;
}

/************************************************************
*
* Function name	: det_set_open_door
* Description	: 设置箱门状态
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_open_door(uint8_t mode)
{
	sg_datacollec_t.open_door = mode;
}

/************************************************************
*
* Function name	: det_get_open_door
* Description	: 获取箱门状态
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t det_get_open_door(void)
{
	return sg_datacollec_t.open_door;
}


/************************************************************
*
* Function name	: det_get_cabinet_posture
* Description	: 获取箱体姿态
* Parameter		: 
* Return		: 
*	
************************************************************/
uint16_t det_get_cabinet_posture(void)
{
	return sg_datacollec_t.attitude_acc;
}

/************************************************************
*
* Function name	: det_set_220v_in_function
* Description	: 更新220v市电输入状态
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_220v_in_function(uint8_t status)
{
	sg_datacollec_t.vinin = status;
}

/************************************************************
*
* Function name	: det_get_220v_in_function
* Description	: 获取220v市电输入状态
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t det_get_220v_in_function(void)
{
	return sg_datacollec_t.vinin;
}

/************************************************************
*
* Function name	: det_set_camera_status 
* Description	: 设置摄像机外设状态
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_camera_status(uint8_t num,uint8_t status)
{
	sg_datacollec_t.camera[num] = status;
}

/************************************************************
*
* Function name	: det_get_camera_status
* Description	: 获取摄像机状态
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t det_get_camera_status(uint8_t num)
{
	return sg_datacollec_t.camera[num];
}

/************************************************************
*
* Function name	: det_set_main_network_status
* Description	: 主网络状态状态设置
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_main_network_status(uint8_t status)
{
	sg_datacollec_t.main_ip = status;
}

/************************************************************
*
* Function name	: det_get_main_network_status
* Description	: 主网络状态获取
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t det_get_main_network_status(void)
{
	return sg_datacollec_t.main_ip;
}

/************************************************************
*
* Function name	: det_set_main_network_sub_status
* Description	: 主网络状态状态设置 - 2
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_main_network_sub_status(uint8_t status)
{
	sg_datacollec_t.main_sub_ip = status;
}

/************************************************************
*
* Function name	: det_get_main_network_sub_status
* Description	: 主网络状态获取 - 2
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t det_get_main_network_sub_status(void)
{
	return sg_datacollec_t.main_sub_ip;
}


/************************************************************
*
* Function name	: det_set_total_voltage
* Description	: 总电压设置函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_total_voltage(float data)
{
	sg_datacollec_t.vin220v = data / RN8207_VOL_KP;
}

/************************************************************
*
* Function name	: det_set_total_current
* Description	: 总电流设置函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_total_current(float data)
{
	sg_datacollec_t.total_current = data / RN8207_CUR_KP;
}
/***********************************************************************
* 功能：设置电流值(外置电流互感器)
* 输入：data:电流有效值
* 输出：无
* 说明： 
***********************************************************************/
void det_set_total_current_b(float data)
{
	sg_datacollec_t.total_current_b = data / RN8209_CUR_B_KP ;//- 5.55;
}
/************************************************************
*
* Function name	: det_set_ping_status
* Description	: 设置ping状态
* Parameter		: 
* Return		: 
*	
************************************************************/
void det_set_ping_status(uint8_t status)
{
	sg_datacollec_t.ping_status = status;
}

/************************************************************
*
* Function name	: det_set_lighting_protector_detection
* Description	: 设置防雷开关状态
* Parameter		: 
*	@mode		: 0=无，1=正常，2=失效
* Return		: 
*	   20230721
************************************************************/
void det_set_spd_status(uint8_t mode)
{
	sg_datacollec_t.spd = mode;
}
/************************************************************
*
* Function name	: det_get_lighting_protector_detection
* Description	: 获取防雷开关状态
* Parameter		: 
* Return		: 
*	  20230721
************************************************************/
uint8_t det_get_spd_status(void)
{
	return sg_datacollec_t.spd;
}


