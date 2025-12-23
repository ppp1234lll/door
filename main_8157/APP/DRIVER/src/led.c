#include "led.h"
#include "delay.h"
/*
		外接电源指示灯  	POWER_OUT 	: PD15
		外接网口指示灯  	LAN_OUT	  	: PD14

		系统状态指示灯     STATE     : PC9
		网口指示灯         LAN       : PA12
		4G指示灯          GPRS       : PA11
		摄像头1           Camera1    : PC8
		摄像头2           Camera2    : PC7
*/

/*
实际指示：
		系统状态指示灯     STATE      : PA12
		网口指示灯         LAN        : PA11
		4G指示灯           GPRS       : PC8
		摄像头1            Camera1    : PC9
		摄像头2            Camera2    : PC7
*/


#define LED_STATE 	PCout(9)
#define LED_LAN   	PAout(12)
#define LED_GPRS	  PAout(11)
#define LED_CAME1 	PCout(8)
#define LED_CAME2   PCout(7)

#define LED_OUT_LAN     PDout(14)
#define LED_OUT_POWER   PDout(15)

#define LED_STATE_TOG   GPIO_ReadOutputDataBit(GPIOC,GPIO_Pin_9) 
#define LED_LAN_STA     GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_12) 
#define LED_GPRS_TOG    GPIO_ReadOutputDataBit(GPIOA,GPIO_Pin_11) 
#define LED_OUT_LAN_TOG GPIO_ReadOutputDataBit(GPIOD,GPIO_Pin_14) 


typedef struct
{
	uint8_t gprs;
	uint8_t lan;
	uint8_t lan_out;
	uint8_t state;
} led_flicker_t;

static led_flicker_t sg_ledflicker_t = {0};

/************************************************************
*
* Function name	: led_gpio_init_function
* Description	: 初始化指示灯控制io:默认不开启
* Parameter		: 
* Return		: 
*	
************************************************************/
void led_gpio_init_function(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD,ENABLE);

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_11|GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_11|GPIO_Pin_12);

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC,&GPIO_InitStructure);
	GPIO_SetBits(GPIOC,GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9);

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD,&GPIO_InitStructure);
	
	LED_OUT_LAN = 0;
	LED_OUT_POWER = 0;
}

/************************************************************
*
* Function name	: led_control_function
* Description	: led灯输出控制函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void led_control_function(LED_DEV dev, LED_STATUS state)
{
	switch(dev)
	{
		case LD_STATE:  // 系统运行指示灯
			switch(state) 
			{
        case LD_ON:	sg_ledflicker_t.state = 2;LED_STATE = state;break;
        case LD_OFF:sg_ledflicker_t.state = 0;LED_STATE = state;break;
        default:		sg_ledflicker_t.state = 1;break;
      }
      break;
		case LD_GPRS:  // 4G指示灯
			switch(state) 
			{
        case LD_ON:			sg_ledflicker_t.gprs = 2;LED_GPRS = state;break;
        case LD_OFF:		sg_ledflicker_t.gprs = 0;LED_GPRS = state;break;
        case LD_FLICKER:sg_ledflicker_t.gprs = 1;break;
        default:break;
			}
			break;
		case LD_LAN:   // 有线指示灯
			switch(state) 
			{
        case LD_ON:			sg_ledflicker_t.lan = 2;LED_LAN = state;break;
        case LD_OFF:		sg_ledflicker_t.lan = 0;LED_LAN = state;break;
        case LD_FLICKER:sg_ledflicker_t.lan = 1;break;
        default:break;
			}
			break;

    case LD_CAME1:	  // 摄像机1
			LED_CAME1 	= state;			break;
    case LD_CAME2:	  // 摄像机2
			LED_CAME2 	= state;			break;
		default:		break;
	}
}

/************************************************************
*
* Function name	: led_outside_control_function
* Description	: 外部led控制
* Parameter		: 
* Return		: 
*	
************************************************************/
void led_out_control_function(LED_DEV dev, LED_STATUS state)
{
	switch(dev)
	{
		case LD_POWER:	
			LED_OUT_POWER = !state;
		break;
		case LD_LAN_LED:
			switch(state) 
			{
				case LD_ON:			sg_ledflicker_t.lan_out = 2;LED_OUT_LAN = 1;break;
				case LD_OFF:		sg_ledflicker_t.lan_out = 0;LED_OUT_LAN = 0;break;
				case LD_FLICKER:sg_ledflicker_t.lan_out = 1;							  break;
				default:break;
			}
			break;
		default:			break;
	}
}

#define FLICKER_TIME 		(500)
#define FLICKER_TIME_1S (1000)
#define FLICKER_TIME_2S (2000)
/************************************************************
*
* Function name	: led_flicker_control_timer_function
* Description	: led闪动
* Parameter		: 
* Return		: 
*	
************************************************************/
void led_flicker_control_timer_function(void)
{
	static uint16_t count   = 0;
	static uint16_t count2	= 0;
	static uint8_t  flag[4] = {0};
	
	count++;
	if(count > FLICKER_TIME)
	{
		count = 0;

		if(sg_ledflicker_t.gprs == 1)		/* 显示无线网络状态 */
		{
			flag[0] = 1;
			LED_GPRS = !LED_GPRS_TOG;
		}
		else if(sg_ledflicker_t.gprs == 2) {}
		else 
		{
			if(flag[0] == 1)
			{
				flag[0] = 0;
				LED_GPRS = LD_OFF;
			}
		}

		if(sg_ledflicker_t.lan == 1)		/* 显示有线网络状态 */
		{
			flag[1] = 1;
			LED_LAN = !LED_LAN_STA;
		} 
		else if(sg_ledflicker_t.lan == 2) {		}
		else
		{
			if(flag[1] == 1)
			{
				flag[1] = 0;
				LED_LAN = LD_OFF;
			}
		}
	
		if(sg_ledflicker_t.state == 1) 	/* 系统状态灯 */
		{
			flag[3] = 1;
			LED_STATE = !LED_STATE_TOG;
		} 
		else if(sg_ledflicker_t.state == 2) {		} 
		else 
		{
			if(flag[3] == 1) 
			{
				flag[3] = 0;
				LED_STATE = LD_OFF;
			}
		}
	}
	
	count2++;
	if(count2>FLICKER_TIME_1S) 
	{
		count2 = 0;
		if(sg_ledflicker_t.lan_out == 1) 	/* 现在主网络状态 */
		{
			flag[2] = 1;
			LED_OUT_LAN = !LED_OUT_LAN_TOG;
		} 
		else if(sg_ledflicker_t.lan_out == 0) 
		{
			if(flag[2] == 1) 
			{
				flag[2] = 0;
				LED_OUT_LAN = 0;
			}
		}
	}
}

/************************************************************
*
* Function name	: led_light_all
* Description	: led灯全亮
* Parameter		:
* Return		:
*
************************************************************/
void led_light_all(void)
{
	GPIO_ResetBits(GPIOA,GPIO_Pin_11|GPIO_Pin_12);
	GPIO_ResetBits(GPIOC,GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9);
}

/************************************************************
*
* Function name	: led_light_off
* Description	: led灯全灭
* Parameter		:
* Return		:
*
************************************************************/
void led_light_off(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_11|GPIO_Pin_12);
	GPIO_SetBits(GPIOC,GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9);
}
/************************************************************
*
* Function name	: led_test
* Description	: led测试
* Parameter		:
* Return		:
*
************************************************************/
void led_test(void)
{
	while(1)
	{
		led_light_all();
		LED_OUT_LAN = 1;
		LED_OUT_POWER = 1;
		delay_ms(1000);
		led_light_off();
		LED_OUT_LAN = 0;
		LED_OUT_POWER = 0;
		delay_ms(1000);	
	}
}


