#ifndef _RELAY_H_
#define _RELAY_H_

#include "sys.h"

typedef enum
{
	RELAY_1 = 0, // 适配器1
	RELAY_2 = 1, // 适配器2
} RELAY_DEV;

typedef enum
{
	RELAY_OFF = 0, // 关闭
	RELAY_ON  = 1, // 打开
} RELAY_STATUS;

/* 函数声明 */

void relay_gpio_init_function(void); // 初始化函数
void relay_control(RELAY_DEV dev, RELAY_STATUS state);
int8_t relay_get_status_function(RELAY_DEV dev);
int8_t relay_get_status(uint8_t dev);
void relay_test(void);
	
#endif
