#include "RN8207C.h"
#include "elec_uart.h"
#include "delay.h"
#include "det.h"

/*
	7、RN8207C单相计量芯片: 串口1，波特率4800，
	   引脚分配为： USART4_TX： PC11
                  USART4_RX： PC10	
*/
/* 确定基表参数：HFConst
HFConst=[16.1079*Vu*Vi*10^11/(EC*Un*Ib)]
Vu:电压通道ADC输入信号，需要乘以增益倍数，一般选择为0.1~0.22V
Vi:电流通道ADC输入信号，需要乘以增益倍数，如5A*（350微欧/10^6）*16=0.028v
EC:电表脉冲常数，自定义，如3200
Un:额定电压，自定义，如220V
Ib:额定电流，自定义，如5A
*/

/* 确定电压、电流、功率转换系数
Kv=Un/V:电压转换系数
Ki=Ib/I:电流转换系数
Kp=3.22155*10^12/(2^32*HFConst*EC):功率转换系数
EC:脉冲常数
Un:实际电压
V:有效值
Ib:实际电流
I:有效值
*/
/*
235.5
*/

#define RN8209_ENABLE

struct rn8207_data_t {
	uint8_t auto_cmd;		// 自动采集功能
	uint8_t sum;				// 和校验 - 主要是读使用
	uint8_t repeat;			// 重复计数
	uint8_t cutdown;		// 超时倒计时 在等于1的状态下，计数清零，标识超时，result使用2
	uint8_t result;			// 结果：0-等待 1-获取到 2-超时
	uint8_t send; 			// 1-有发送数据
	uint8_t reg;				// 本次操作类型-寄存器地址			
	uint8_t mode;				// 0-读 0x80-写
	uint8_t flag;  		// 采集标志位  0:电压  1、电流A  2、电流B
};

static uint16_t sg_rn8207_rec_sta = 0;
static uint8_t  sg_rn8207_buff[ELEC_RX_MAX] = {0};
struct rn8207_data_t sg_rn8207data_t = {0};

/*
* 地址：0、串口模式
* 波特率固定：4800
* 含偶校验
*/

/* 接口与参数 */
#define RN8207_BAUDRATE (4800)
#define RN8207_USART_INIT(baudrate) elec_uart_init(baudrate)
#define RN8207_SEND_STR(buff,len) 	elec_send_str_function(buff,len)
#define RN8207_CHIPID   (0x88)

/* 宏定义数据 */
#define RN8209_DET_NUM   			3  			// 采集次数 
#define RN8207_COM_CUTDONW  	200 		// 200ms
#define RN8207_AUTO_TIME   		1000 	  // 1s
#define RN8207_HFCONST				0x1000  // 脉冲频率寄存器


/* 数据 */
#define RN8207_REC_STA  sg_rn8207_rec_sta
#define RN8207_REC_BUFF sg_rn8207_buff


/************************************************************
*
* Function name	: rn8207_reset_function
* Description	: 复位函数
* Parameter		: 
* Return		: 
*	      先将 RX 引脚置低 25ms，然后再将 RX 引脚置高 20ms
************************************************************/
void rn8207_reset_function(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(ELEC_RX_GPIO_CLK,ENABLE);

	GPIO_InitStructure.GPIO_Pin   = ELEC_RX_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_Init(ELEC_RX_GPIO_PORT,&GPIO_InitStructure);

	GPIO_WriteBit(ELEC_RX_GPIO_PORT,ELEC_RX_PIN,Bit_RESET);
	delay_ms(30);
	GPIO_WriteBit(ELEC_RX_GPIO_PORT,ELEC_RX_PIN,Bit_SET);
}


/************************************************************
*
* Function name	: rn8207c_init_function
* Description	: 初始化                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207c_init_function(void)
{
	rn8207_reset_function();
	RN8207_USART_INIT(RN8207_BAUDRATE);
	
	/* 开启自动采集功能-电压、电流 */
	sg_rn8207data_t.auto_cmd = 1;
	
	/* 写命令使能 */
	rn8207_write_enable_function(1);
	
	rn8207_set_hfconst_function();
	rn8207_system_control_function(VG_PGA1,CG_PGA1,CG_PGAIB1);
	
	/* 写命令失能 */
	rn8207_write_enable_function(0);
}

/************************************************************
*
* Function name	: rn8207_sending_data_function
* Description	: 发送数据函数
* Parameter		: 
*	@reg		: 寄存器值
*	@mode		: 发送模式
* Return		: 
*	
************************************************************/
void rn8207_sending_data_function(uint8_t reg, uint8_t mode)
{
	sg_rn8207data_t.cutdown = 1;
	sg_rn8207data_t.result  = 0;
	sg_rn8207data_t.mode    = mode;
	sg_rn8207data_t.send    = 1;
	sg_rn8207data_t.reg     = reg;
	sg_rn8207data_t.repeat  = 0;
}

/************************************************************
*
* Function name	: rn8207_send_over_function
* Description	: 数据发送操作完成
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_send_over_function(void)
{
	sg_rn8207data_t.cutdown = 0;
	sg_rn8207data_t.result  = 0;
	sg_rn8207data_t.send    = 0;
	sg_rn8207data_t.repeat  = 0;
}

/************************************************************
*
* Function name	: rn8207_deal_read_data_function
* Description	: 处理读取到的数据
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t rn8207_deal_read_data_function(void)
{
	uint32_t data  = 0;
	int8_t   ret   = 0;
	uint8_t  index = 0;
	
	switch(sg_rn8207data_t.reg) 
	{
		case CURRENT_A_RMS_REG:
			
			for(index=0; index<3; index++) 
				sg_rn8207data_t.sum += RN8207_REC_BUFF[index];
			
			if(RN8207_REC_BUFF[3] != (0xff-sg_rn8207data_t.sum)) 
			{
				ret = -1;
				break;
			}
			data = RN8207_REC_BUFF[0];
			data = (data<<8) | RN8207_REC_BUFF[1];
			data = (data<<8) | RN8207_REC_BUFF[2];
			
			if(data & 0x800000) 
				det_set_total_current(0);
			else
				det_set_total_current(data);
			break;
			
		case VOLTAGE_RMS_REG:
			for(index=0; index<3; index++) 
				sg_rn8207data_t.sum += RN8207_REC_BUFF[index];
			
			if(RN8207_REC_BUFF[3] != (0xff-sg_rn8207data_t.sum)) 
			{
				ret = -1;
				break; 
			}
			data = RN8207_REC_BUFF[0];
			data = (data<<8) | RN8207_REC_BUFF[1];
			data = (data<<8) | RN8207_REC_BUFF[2];
			if(data & 0x800000)
				det_set_total_voltage(0);
			else 
				det_set_total_voltage(data);
		break;

#ifdef RN8209_ENABLE
		case CURRENT_B_RMS_REG:  // 电流通道B
			for(index=0; index<3; index++)   // 计算校验值
				sg_rn8207data_t.sum += RN8207_REC_BUFF[index];
	
			if(RN8207_REC_BUFF[3] != (0xff-sg_rn8207data_t.sum)) // 判断校验值
			{
				ret = -1;
				break;
			}
			data = RN8207_REC_BUFF[0];
			data = (data<<8) | RN8207_REC_BUFF[1];
			data = (data<<8) | RN8207_REC_BUFF[2];
			if(data & 0x800000) 
				det_set_total_current_b(0);
			else 
				det_set_total_current_b(data);
			break;
#endif	
		
		default:			break;
	}
	return ret;
}

/************************************************************
*
* Function name	: rn8207_repeat_function
* Description	: 重复操作函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_repeat_function(void)
{
	/* 数据等待超时 */
	if( (++sg_rn8207data_t.repeat) <= 3) 
	{
		sg_rn8207data_t.cutdown = 1;	/* 重复获取或写入 */
		rn8207_read_reg_function(sg_rn8207data_t.reg,1);
	} 
	else 
		rn8207_send_over_function();		/* 结束任务 */
}

/************************************************************
*
* Function name	: rn8207_analysis_data_function
* Description	: 数据读取函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_analysis_data_function(void)
{
	int8_t   ret = 0;
	/* 等待回传数据 */
	if(RN8207_REC_STA&0x8000) 
	{
		if(sg_rn8207data_t.mode == CMD_READ_REG) 		/* 数据处理 */
		{
			ret = rn8207_deal_read_data_function();		/* 读取数据 */
			if(ret != 0) 
				rn8207_repeat_function();
			else 
				rn8207_send_over_function();
		}
		RN8207_REC_STA = 0;
	}
	
	/* 检测本次操作是否超时 */
	if(sg_rn8207data_t.result == 2) 
	{
		sg_rn8207data_t.result = 0;
		rn8207_repeat_function();
	}
}


/************************************************************
*
* Function name	: rn8207_write_reg_function
* Description	: 写寄存器
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_write_reg_function(uint8_t reg, uint8_t mode,uint8_t *data, uint8_t len)
{
	uint8_t buff[64] = {0};
	uint8_t index = 0;	
	
#ifndef RN8209_ENABLE
	buff[0] = RN8207_CHIPID;
	buff[1] = CMD_WRITE_REG|(reg&0x7f);
	for(index=0; index<len; index++) 
		buff[2+index] = data[index];
	
	/* 计数和校验 */
	sg_rn8207data_t.sum = 0;
	for(index=0; index<(2+len); index++)
		sg_rn8207data_t.sum+=buff[index];
	
	/* 填充和校验 */
	buff[2+len] = 0xff - sg_rn8207data_t.sum;
	
	/* 更新标志 */
	if( mode == 0)
		rn8207_sending_data_function(reg,CMD_WRITE_REG);
	
	/* 数据发送 */
	RN8207_SEND_STR(buff,3+len);
#else
	
	buff[0] = CMD_WRITE_REG|(reg&0x7f);
	for(index=0; index<len; index++) {
		buff[1+index] = data[index];
	}
	sg_rn8207data_t.sum = 0;	/* 计数和校验 */
	for(index=0; index<(1+len); index++) {
		sg_rn8207data_t.sum+=buff[index];
	}
	buff[1+len] = 0xff - sg_rn8207data_t.sum;/* 填充和校验 */
	if( mode == 0) 	/* 更新标志 */
		rn8207_sending_data_function(reg,CMD_WRITE_REG);
	RN8207_SEND_STR(buff,2+len);	/* 数据发送 */
#endif	
}
/************************************************************
*
* Function name	: rn8207_read_reg_function
* Description	: 寄存器读取命令
* Parameter		: 
*	@reg		: 寄存器值
*	@mode		: 0-更新标志 other-不更新
* Return		: 
*	
************************************************************/
void rn8207_read_reg_function(uint8_t reg, uint8_t mode)
{
	uint8_t buff[2] = {0};
#ifndef RN8209_ENABLE	
	/* 数据 */
	buff[0] = RN8207_CHIPID;
	buff[1] = CMD_READ_REG|(reg&0x7f);
	
	/* 计数和校验 */
	sg_rn8207data_t.sum = 0;
	sg_rn8207data_t.sum += buff[0];
	sg_rn8207data_t.sum += buff[1];
	
	/* 更新标志 */
	if( mode == 0) 
		rn8207_sending_data_function(reg,CMD_READ_REG);
	RN8207_SEND_STR(buff,sizeof(buff));	/* 数据发送 */
#else

	buff[0] = CMD_READ_REG|(reg&0x7f);	/* 数据 */
	sg_rn8207data_t.sum = buff[0];   /* 计数和校验 */

	if( mode == 0) 	/* 更新标志 */
	rn8207_sending_data_function(reg,CMD_READ_REG);
	RN8207_SEND_STR(buff,sizeof(buff));	/* 数据发送 */
#endif
}

/************************************************************
*
* Function name	: rn8207_send_data_function
* Description	: 数据发送函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_send_data_function(void)
{
	/* 允许进行发送操作 */
	if(sg_rn8207data_t.send == 0) 
	{
		#if RN8207_TEST_FLAG
		if(sg_rn8207data_t.flags.syscontrol) 
		{
			sg_rn8207data_t.flags.syscontrol = 0;
			rn8207_read_reg_function(SYSTEM_CONTROL_REG,0);
			return;
		}
		if(sg_rn8207data_t.flags.sysstatus) 
		{
			sg_rn8207data_t.flags.sysstatus = 0;
			rn8207_read_reg_function(SYSTEM_STATUS_REG,0);
			return;
		}
		#endif
		if(sg_rn8207data_t.flag > 0)
		{
			switch(sg_rn8207data_t.flag)
			{
				case 1: rn8207_read_reg_function(VOLTAGE_RMS_REG,0); 	 break;
				case 2: rn8207_read_reg_function(CURRENT_A_RMS_REG,0); break;
				case 3: rn8207_read_reg_function(CURRENT_B_RMS_REG,0); break;
				default: break;			
			}
			sg_rn8207data_t.flag--;

		}
	}
}

/************************************************************
*
* Function name	: rn8207_run_timer_function
* Description	: 运行计时相关函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_run_timer_function(void)
{
	static uint16_t times = 0;
	static uint16_t  auto_time = 0;
	
	if(sg_rn8207data_t.cutdown != 0) 	/* 计数更新 */
	{
		if(sg_rn8207data_t.cutdown == 1) 
		{
			sg_rn8207data_t.cutdown = 2;
			times = 0;
		}
		/* 计数值 */
		if((++times) >= RN8207_COM_CUTDONW) 
		{
			times = 0;
			sg_rn8207data_t.cutdown = 0;
			sg_rn8207data_t.result = 2;
		}
	} 
	else 
		times = 0;
	
	/* 自动采集 */
	if(sg_rn8207data_t.auto_cmd == 1) 
	{
		if((++auto_time) >= RN8207_AUTO_TIME) 
		{
			auto_time = 0;
			sg_rn8207data_t.flag = RN8209_DET_NUM;
		}
	} 
	else 
		auto_time = 0;

}

/************************************************************
*
* Function name	: rn8207_work_process_function
* Description	: 工作进程函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_work_process_function(void)
{
	rn8207_analysis_data_function();
	rn8207_send_data_function();
}

/************************************************************
*
* Function name	: rn8207_get_rec_data_function
* Description	: 获取通信数据
* Parameter		: 
*	@
* Return		: 
*	
************************************************************/
void rn8207_get_rec_data_function(uint8_t *buff, uint16_t len)
{
	uint16_t index = 0;

	if(buff == NULL || len == 0) {
		return;
	}
	
	for(index=0; index<len; index++) {
		RN8207_REC_BUFF[index] = buff[index];
	}
	
	sg_rn8207_rec_sta = 0x8000|len;
}

/************************************************************
*
* Function name	: rn8207_write_enable_function
* Description	: 写使能控制函数
* Parameter		: 
*	@cmd		: 0-失能 1-使能
* Return		: 
*	
************************************************************/
void rn8207_write_enable_function(uint8_t cmd)
{
	uint8_t buff[2] = {0};
	
	if(cmd == 1) {
		buff[0] = 0xEA;
		buff[1] = 0xE5;
		rn8207_write_reg_function(buff[0],0,&buff[1],1);
	} else {
		buff[0] = 0xEA;
		buff[1] = 0xDC;
		rn8207_write_reg_function(buff[0],0,&buff[1],1);
	}
}

/************************************************************
*
* Function name	: rn8207_system_control_function
* Description	: 系统控制寄存器
* Parameter		: 
* Return		: 
*	
************************************************************/
void rn8207_system_control_function(uint8_t vol_gain,uint8_t cur_gain,uint8_t curb_gain)
{
	uint8_t buff[2] = {0};
	
	buff[0] = 0x00;
	buff[1] = 0x00;
	switch(vol_gain) 
	{
		case 1:buff[1] |= (0<<2);break;
		case 2:buff[1] |= (1<<2);break;
		case 4:buff[1] |= (2<<2);break;
		default:break;
	}
	switch(cur_gain) 
	{
		case 1:buff[1] |= 0;break;
		case 2:buff[1] |= 1;break;
		case 4:buff[1] |= 2;break;
		default:break;
	}
#ifdef RN8209_ENABLE
	switch(curb_gain) 
	{
		case 1:buff[1] |= (0<<4);break;
		case 2:buff[1] |= (1<<4);break;
		case 4:buff[1] |= (2<<4);break;
		default:break;
	}
	buff[1] |= (1<<6);   // 开启电流通道B
#endif
	rn8207_write_reg_function(SYSTEM_CONTROL_REG,0,buff,2);
}

/************************************************************
*
* Function name	: rn8207_set_hfconst_function
* Description	: 设置脉冲频率寄存器
* Parameter		: 
* Return		: 
*	
************************************************************/
static void rn8207_set_hfconst_function(void)
{
	uint8_t buff[2] = {0};
	
	buff[0] = (RN8207_HFCONST>>8)&0xff;
	buff[1] = (RN8207_HFCONST)&0xff;
	rn8207_write_reg_function(HIGH_FREQ_REG,0,buff,2);
	
}


/************************************************************
*
* Function name	: rn8207_test
* Description	: 电压、电流测试
* Parameter		:
* Return		:
*
************************************************************/
void rn8207_test(void)
{
	while(1)
	{
		rn8207_work_process_function();	// 数据获取函数
		delay_ms(200);		
	}
}






