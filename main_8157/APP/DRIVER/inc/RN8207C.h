#ifndef _RN8207C_H_
#define _RN8207C_H_
#include "bsp.h"

/* 寄存器表单 */
#define SYSTEM_CONTROL_REG 	   0x00 // 系统控制寄存器
#define HIGH_FREQ_REG		   		 0x02 // 脉冲频率寄存器
#define CURRENT_A_RMS_REG      0x22 // 电流A有效值寄存器
#define CURRENT_B_RMS_REG      0x23 // 电流B有效值寄存器
#define VOLTAGE_RMS_REG        0x24 // 电压有效值寄存器
#define SYSTEM_STATUS_REG	     0x43 // 系统状态寄存器

#define RN8207_VOL_KP	     7455   // 电压系数 - v
#define RN8207_CUR_KP		   842    // 电流系数 - ma

#define RN8209_CUR_B_KP		 842    // 电流系数 - ma
// 1134 1171 1226 1048 1046
/* 设置量 */
enum voltage_gain // 电压通道模拟增益选择
{
	VG_PGA1 = 1, // 1倍
	VG_PGA2, // 2倍
	VG_PGA4, // 4倍
};

enum current_gain // 电流通道模拟增益选择
{ 
	CG_PGA1 = 1,  // 1倍
	CG_PGA2,  // 2倍
	CG_PGA8,  // 8倍
	CG_PGA16, // 16倍
};

enum current_b_gain // 电压通道模拟增益选择
{
	CG_PGAIB1 = 1, // 1倍
	CG_PGAIB2, // 2倍
	CG_PGAIB4, // 4倍
};


#define CMD_WRITE_REG (0x80)
#define CMD_READ_REG  (0x00)

void rn8207_reset_function(void);
void rn8207c_init_function(void);

void rn8207_sending_data_function(uint8_t reg, uint8_t mode);
void rn8207_send_over_function(void);
int8_t rn8207_deal_read_data_function(void);
void rn8207_repeat_function(void);
void rn8207_analysis_data_function(void);
void rn8207_write_reg_function(uint8_t reg, uint8_t mode,uint8_t *data, uint8_t len);
void rn8207_read_reg_function(uint8_t reg, uint8_t mode);
void rn8207_send_data_function(void);
void rn8207_run_timer_function(void);
void rn8207_work_process_function(void);
void rn8207_get_rec_data_function(uint8_t *buff, uint16_t len);
void rn8207_write_enable_function(uint8_t cmd);
void rn8207_system_control_function(uint8_t vol_gain,uint8_t cur_gain,uint8_t curb_gain);
void rn8207_set_hfconst_function(void);

void rn8207_test(void);

#endif
