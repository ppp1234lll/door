#include "relay.h"
#include "led.h"
#include "delay.h"

/*
	3、继电器

		摄像头1           Camera1     : PB7
		摄像头2           Camera2     : PB6
*/

#define RELAY1_GPIO_CLK				RCC_APB2Periph_GPIOB
#define RELAY1_GPIO_PORT      GPIOB
#define RELAY1_PIN            GPIO_Pin_7

#define RELAY2_GPIO_CLK				RCC_APB2Periph_GPIOB
#define RELAY2_GPIO_PORT      GPIOB
#define RELAY2_PIN            GPIO_Pin_6

#define RELAY1_CTRL   PBout(7)
#define RELAY2_CTRL   PBout(6)

typedef struct
{
	uint8_t relay_1; 
	uint8_t relay_2; 
} relay_t;

relay_t sg_relay_states_t;
/************************************************************
*
* Function name	: relay_gpio_init_function
* Description	: 继电器初始化
* Parameter		: 
* Return		: 
*	
************************************************************/
void relay_gpio_init_function(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RELAY1_GPIO_CLK|RELAY2_GPIO_CLK,ENABLE);

	GPIO_InitStructure.GPIO_Pin   = RELAY1_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(RELAY1_GPIO_PORT,&GPIO_InitStructure); 

	GPIO_InitStructure.GPIO_Pin   = RELAY2_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(RELAY2_GPIO_PORT,&GPIO_InitStructure); 
	
//	relay_control(RELAY_1,RELAY_OFF);
//	relay_control(RELAY_2,RELAY_OFF);
}


/************************************************************
*
* Function name	: relay_control
* Description	: 继电器控制
* Parameter		: 
* Return		: 
*	
************************************************************/
void relay_control(RELAY_DEV dev, RELAY_STATUS state)
{
	switch(dev)
	{
		case RELAY_1:
			sg_relay_states_t.relay_1 = state;
			RELAY1_CTRL = (state?0:1);
			if(state == RELAY_ON)
				led_control_function(LD_CAME1,LD_ON);
			else
				led_control_function(LD_CAME1,LD_OFF);
			break;
			
		case RELAY_2:
			sg_relay_states_t.relay_2 = state;
			RELAY2_CTRL = (state?0:1);
			if(state == RELAY_ON)
				led_control_function(LD_CAME2,LD_ON);
			else
				led_control_function(LD_CAME2,LD_OFF);
			break;
			
		default:
			break;
	}
}

/************************************************************
*
* Function name	: relay_get_status_function
* Description	:  获取继电器状态
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t relay_get_status_function(RELAY_DEV dev)
{
	switch(dev)
	{
		case RELAY_1:
			return  (sg_relay_states_t.relay_1?1:0);
		case RELAY_2:
			return  (sg_relay_states_t.relay_2?1:0);
		default:
			break;
	}
	return 0;
}


/************************************************************
*
* Function name	: relay_get_status
* Description	:  获取继电器状态
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t relay_get_status(uint8_t dev)
{
	switch(dev)
	{
		case 0:
			if(sg_relay_states_t.relay_1)	return 1;
			else	return 2;
		case 1:
			if(sg_relay_states_t.relay_2)	return 1;
			else	return 2;
		
		default:
			break;
	}
	return 0;
}

/************************************************************
*
* Function name	: relay_test
* Description	: 继电器测试
* Parameter		:
* Return		:
*
************************************************************/
void relay_test(void)
{
	while(1)
	{
		relay_control(RELAY_1,RELAY_ON); // 开继电器1
		relay_control(RELAY_2,RELAY_ON); // 开继电器2
		delay_ms(2000);
		relay_control(RELAY_1,RELAY_OFF); // 关继电器1
		relay_control(RELAY_2,RELAY_OFF); // 关继电器2
		delay_ms(2000);	
	}
}


