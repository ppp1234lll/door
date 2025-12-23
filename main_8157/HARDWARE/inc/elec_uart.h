/********************************************************************************
* @File name  : uart4.h
* @Description: 
* @Author     : ZHLE
*  Version Date        Modification Description
	7、RN8207C单相计量芯片: 串口4，波特率4800，
	   引脚分配为    USART4_TX： PC10
									USART4_RX：  PC11
********************************************************************************/
#ifndef _ELEC_UART_H_
#define _ELEC_UART_H_
#include "sys.h"

/* 宏定义 */
#define ELEC_TX_GPIO_CLK				       RCC_APB2Periph_GPIOC
#define ELEC_TX_GPIO_PORT              GPIOC
#define ELEC_TX_PIN                    GPIO_Pin_10

#define ELEC_RX_GPIO_CLK				       RCC_APB2Periph_GPIOC
#define ELEC_RX_GPIO_PORT              GPIOC
#define ELEC_RX_PIN                    GPIO_Pin_11

#define ELEC_USART_CLK                 RCC_APB1Periph_UART4
#define ELEC_USART                     UART4
#define ELEC_IRQn               			 UART4_IRQn
#define ELEC_IRQHandler				 				 UART4_IRQHandler

/* 参数 */
#define ELEC_RX_MAX 100

/* 函数声明 */
void elec_uart_init(uint32_t bound);

void elec_send_str_function(uint8_t *data, uint16_t len);

#endif
