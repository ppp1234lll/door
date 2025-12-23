#include "dma.h"

/************************************************************
*
* Function name		: DMA_Config
* Description		: DMA配置函数
* Parameter			: 
*	@chx			: DMA通道选择,@ref DMA_channel DMA1_Channel_0~DMA2_Channel_7
*	@par			: 外设地址
*	@mar			: 存储器地址
*	@dir			: 传输方向
*	@ndtr			: 数据传输量  
* Return			: 
*	
************************************************************/
void DMA_Config(DMA_Channel_TypeDef* chx,uint32_t par,uint32_t mar,uint32_t dir,uint16_t ndtr)
{ 
	DMA_InitTypeDef  DMA_InitStructure;
	
	if(chx <= DMA2_Channel1) {
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	} else {
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);
	}
	DMA_DeInit(chx);
	
	DMA_InitStructure.DMA_PeripheralBaseAddr 	= par;  						// DMA外设基地址
	DMA_InitStructure.DMA_MemoryBaseAddr 		= mar;  						// DMA内存基地址
	DMA_InitStructure.DMA_DIR 					= dir;  						// 数据传输方向，从内存读取发送到外设
	DMA_InitStructure.DMA_BufferSize 			= ndtr; 						// DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_PeripheralInc 		= DMA_PeripheralInc_Disable;  	// 外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc 			= DMA_MemoryInc_Enable;  		// 内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize 	= DMA_PeripheralDataSize_Byte;  // 数据宽度为8位
	DMA_InitStructure.DMA_MemoryDataSize 		= DMA_MemoryDataSize_Byte; 		// 数据宽度为8位
	DMA_InitStructure.DMA_Mode 					= DMA_Mode_Normal;  			// 工作在正常缓存模式
	DMA_InitStructure.DMA_Priority 				= DMA_Priority_Medium; 			// DMA通道 x拥有中优先级 
	DMA_InitStructure.DMA_M2M 					= DMA_M2M_Disable;  			// DMA通道x没有设置为内存到内存传输
	DMA_Init(chx, &DMA_InitStructure);  
	
	DMA_Cmd(chx,ENABLE);
} 

/************************************************************
*
* Function name		: DMA_Enable
* Description		: 开启一次DMA传输
* Parameter			: 
*	@DMA_Streamx	: DMA数据流,DMA1_Stream0~7/DMA2_Stream0~7
*	@ndtr			: 数据传输量  
* Return			: 
*	
************************************************************/
void DMA_Enable(DMA_Channel_TypeDef *chx,uint16_t ndtr)
{
	DMA_Cmd(chx, DISABLE);      // 先关闭DMA,才能设置它
	chx->CNDTR = ndtr;					// 设置发送长度
	DMA_Cmd(chx, ENABLE);       // 开启DMA
}	  

