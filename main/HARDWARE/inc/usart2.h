#ifndef _USART2_H_
#define _USART2_H_

#include "sys.h"

#define USART2_RX_MODE (1) // 0-中断字节模式 1-DMA空闲中断模式
#define USART2_RX_MAX 100

void usart2_init_function(uint32_t baudrate);
void usart2_send_str(uint8_t *buff, uint16_t len);

#endif
