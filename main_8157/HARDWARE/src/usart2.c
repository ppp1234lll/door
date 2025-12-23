#include "usart2.h"
#include "stdio.h"

uint8_t usart2_rx_buff[USART2_RX_MAX] = {0};

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;   
//__use_no_semihosting was requested, but _ttywrch was
void _ttywrch(int ch)
{
ch = ch;
}
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{  
	USART2->DR = (int8_t) ch;
	while((USART2->SR&0X40)==0);
	return ch;
}
#endif 


/************************************************************
*
* Function name	: usart2_init_function
* Description	: 串口2初始化函数
* Parameter		: 
*	@bau
* Return		: 
*	
************************************************************/
void usart2_init_function(uint32_t baudrate)
{	
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* 全映射 */
	GPIO_PinRemapConfig(GPIO_Remap_USART2,ENABLE);
	/* 时钟初始化 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD  , ENABLE); 
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 , ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);	

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;		 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_Init(GPIOD, &GPIO_InitStructure);				 
	
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			  
	NVIC_Init(&NVIC_InitStructure);						
	
	USART_DeInit(USART2);
	
	USART_InitStructure.USART_BaudRate = baudrate; // 波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b; // 字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1; // 一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No; // 无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	// 收发模式
	USART_Init(USART2, &USART_InitStructure);	   									
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE); 									// 开启串口接受中断

	USART_ClearITPendingBit(USART2, USART_IT_TC);
	USART_Cmd(USART2, ENABLE);					   									// 使能串口3
	
}

/************************************************************
*
* Function name	: usart2_send_char
* Description	: 发送一个字节
* Parameter		: 
*	@ch			: 待发送的字节数据
* Return		: 
*	
************************************************************/
void usart2_send_char(uint8_t ch)
{
	USART2->DR = (uint8_t)ch;
	while ((USART2->SR & 0X40) == 0);
}

/************************************************************
*
* Function name	: usart2_send_str
* Description	: 发送字符串
* Parameter		: 
*	@buff		: 字符串指针
*	@len		: 发送数据长度
* Return		: 
*	
************************************************************/
void usart2_send_str(uint8_t *buff, uint16_t len)
{
	while(len--) {
		usart2_send_char(buff[0]);
		buff++;
	}
}

/************************************************************
*
* Function name	: USART2_IRQHandler
* Description	: 串口2中断函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void USART2_IRQHandler(void)
{
	static uint8_t test = 0;
	uint8_t res = 0;
	
	if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
		USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		res = USART_ReceiveData(USART2);
		usart2_rx_buff[test++] = res;
	}
}



