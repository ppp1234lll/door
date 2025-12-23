#ifndef _DMA_H_
#define _DMA_H_
#include "bsp.h"

/* 参数 */

/* 函数声明 */

/** 执行函数 **/



/** 数据输入函数 **/
void DMA_Config(DMA_Channel_TypeDef* chx,uint32_t par,uint32_t mar,uint32_t dir,uint16_t ndtr);
void DMA_Enable(DMA_Channel_TypeDef *chx,uint16_t ndtr);

/** 数据输出函数 **/

#endif
