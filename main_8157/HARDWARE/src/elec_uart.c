/********************************************************************************
* @File name  : 电能计量通信驱动
* @Description: 串口4
* @Author     : ZHLE
*  Version Date        Modification Description
	7、RN8207C单相计量芯片: 串口4，波特率4800，
	   引脚分配为： USART2_TX： PC10
		              USART2_RX： PC11
********************************************************************************/

#include "elec_uart.h"
#include "rn8207c.h"
#include "dma.h"

#define ELEC_RX_DMA  
#define ELEC_TX_DMA_CH               	 DMA2_Channel5
#define ELEC_RX_DMA_CH                 DMA2_Channel3

uint8_t elec_recv_buff[ELEC_RX_MAX] = {0};

/************************************************************
*
* Function name	: usart4_init
* Description	: 串口4初始化函数
* Parameter		: 
*	@bound		: 波特率
* Return		: 
*	
************************************************************/
void elec_uart_init(u32 bound)
{
	/* GPIO端口设置 */
	GPIO_InitTypeDef  GPIO_InitStructure  = {0};
	USART_InitTypeDef USART_InitStructure = {0};
	NVIC_InitTypeDef  NVIC_InitStructure  = {0};

	RCC_APB2PeriphClockCmd(ELEC_TX_GPIO_CLK|ELEC_RX_GPIO_CLK, ENABLE);  
	RCC_APB1PeriphClockCmd(ELEC_USART_CLK,ENABLE);
	
	/* USART端口配置 */
	GPIO_InitStructure.GPIO_Pin   = ELEC_TX_PIN; 	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_AF_PP;
	GPIO_Init(ELEC_TX_GPIO_PORT,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin   = ELEC_RX_PIN; 	
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode 	= GPIO_Mode_IN_FLOATING;
	GPIO_Init(ELEC_RX_GPIO_PORT,&GPIO_InitStructure);
	
	/* USART 初始化设置 */
	USART_DeInit(ELEC_USART);
	USART_InitStructure.USART_BaudRate = bound;									// 串口波特率
	USART_InitStructure.USART_WordLength = USART_WordLength_9b;						// 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;							// 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_Even;							// 偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;					// 收发模式
	USART_Init(ELEC_USART, &USART_InitStructure);	 

	/* Usart NVIC 配置 */
	NVIC_InitStructure.NVIC_IRQChannel = ELEC_IRQn; // 串口中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 2; // 抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1 ; // 子优先级1
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; // IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	

	#ifdef ELEC_RX_DMA
	DMA_Config( ELEC_RX_DMA_CH,(uint32_t)&(ELEC_USART->DR),(uint32_t)elec_recv_buff,DMA_DIR_PeripheralSRC,ELEC_RX_MAX);
	USART_ITConfig(ELEC_USART, USART_IT_IDLE, ENABLE); // 开启空闲中断
	USART_DMACmd(ELEC_USART, USART_DMAReq_Rx, ENABLE);
	#else
	USART_ITConfig(ELEC_USART, USART_IT_RXNE, ENABLE); // 开启相关中断
	#endif
	
	USART_ClearFlag(ELEC_USART, USART_FLAG_TC);
	USART_Cmd(ELEC_USART, ENABLE); // 使能串口4
}

/************************************************************
*
* Function name	: elec_send_char_function
* Description	: 串口4-字节发送函数
* Parameter		: 
*   @ch         : 需要发送的字节
* Return		: 
*	
************************************************************/
void elec_send_char_function(uint8_t ch)
{
	ELEC_USART->DR = (int8_t) ch;
	while((ELEC_USART->SR&0X40)==0);
}

/************************************************************
*
* Function name	: uart4_send_str_function
* Description	: 串口4-字符串发送函数
* Parameter		: 
*   @data       : 数据
*   @len        : 长度
* Return		: 
*	
************************************************************/
void elec_send_str_function(uint8_t *data, uint16_t len)
{
	while(len--) {
		elec_send_char_function(data[0]);
		data++;
	}
}

/************************************************************
*
* Function name	: UART4_IRQHandler
* Description	: 串口4-中断函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void ELEC_IRQHandler(void)
{
	#ifdef ELEC_RX_DMA
	uint16_t size = 0;

	/* 空闲中断接收 */
	if( USART_GetITStatus(ELEC_USART,USART_IT_IDLE) != RESET ) 
	{
		USART_ClearITPendingBit(ELEC_USART,USART_IT_IDLE);
		USART_ReceiveData(ELEC_USART);

		DMA_Cmd(ELEC_RX_DMA_CH, DISABLE); // 关闭DMA，准备重新配置
		size = ELEC_RX_MAX - DMA_GetCurrDataCounter(ELEC_RX_DMA_CH); // 计算接收数据长度					
		rn8207_get_rec_data_function(elec_recv_buff,size); // 填充接收到的数据		
		DMA_Enable(ELEC_RX_DMA_CH,ELEC_RX_MAX);
	}
	#else
	static uint8_t test = 0;
	uint8_t Res = 0;
	if(USART_GetITStatus(ELEC_USART,USART_IT_RXNE)!=RESET)
	{
		USART_ClearITPendingBit(ELEC_USART,USART_IT_RXNE);
		Res = USART_ReceiveData(ELEC_USART);
		elec_recv_buff[test++] = Res;
	}
	#endif
}



