#ifndef _LED_H_
#define _LED_H_

#include "sys.h"

/* 参数 */
typedef enum
{
	LD_STATE   = 0, // 系统指示灯
	LD_GPRS    = 1, // 2G指示灯
	LD_LAN     = 2, // 网口
//	LD_FILL    = 3, // 闪光灯
//	LD_FAN     = 4, // 风扇
//	LD_NETDEV  = 5, // 加热
	LD_POWER   = 6, // 电源-外接
	LD_LAN_LED = 7, // 网口-外接
	LD_CAME1   = 8, // 网口-外接
	LD_CAME2   = 9, // 网口-外接
} LED_DEV;

typedef enum
{
	LD_ON  	   = 0, // 常亮
	LD_OFF 	   = 1, // 熄灭
	LD_FLICKER = 2, // 闪烁
	LD_FLIC_Q  = 3, // 快速闪烁
} LED_STATUS;

/* 函数声明 */
void led_gpio_init_function(void);	// 初始化函数
void led_flicker_control_timer_function(void);

void led_control_function(LED_DEV dev, LED_STATUS state);
void led_out_control_function(LED_DEV dev, LED_STATUS state);
void led_light_all(void);
void led_light_off(void);
void led_test(void);
#endif
