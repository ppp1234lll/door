#include "spi.h"
#include "delay.h"
#include "includes.h"

/*
	10、W25Q128存储芯片：(硬件SPI方式)，引脚分配为：
		MOSI:   PB5
		MISO:   PB4
		CLK:    PB3
		CS:     PD7
*/


#define SPI_FLASH_SCK		PBout(3)
#define SPI_FLASH_MOSI	PBout(5)
#define SPI_FLASH_MISO	PBin(4)
 	    					
							
/************************************************************
*
* Function name	: SPI_INIT
* Description	: spi初始化函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void w25qxx_SPI_INIT(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB,ENABLE);
	
//	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3|GPIO_Pin_5;
//	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB,&GPIO_InitStructure); 	

//	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
//	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
//	SPI_FLASH_MOSI = 1; 
//	SPI_FLASH_MOSI = 0;

	RCC_APB1PeriphClockCmd(	RCC_APB1Periph_SPI3,  ENABLE );//SPI2时钟使能

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  //PB13/14/15复用推挽输出 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化GPIOB

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;  //设置SPI单向或者双向的数据模式:SPI设置为双线双向全双工
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;		//设置SPI工作模式:设置为主SPI
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;		//设置SPI的数据大小:SPI发送接收8位帧结构
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;		//串行同步时钟的空闲状态为高电平
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;	//串行同步时钟的第二个跳变沿（上升或下降）数据被采样
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;		//NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;		//定义波特率预分频的值:波特率预分频值为256
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;	//指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
	SPI_InitStructure.SPI_CRCPolynomial = 7;	//CRC值计算的多项式
	SPI_Init(SPI3, &SPI_InitStructure);  //根据SPI_InitStruct中指定的参数初始化外设SPIx寄存器
 
	SPI_Cmd(SPI3, ENABLE); //使能SPI外设
	
	
}

/************************************************************
*
* Function name	: SPI_ReadWriteByte
* Description	: 读写字节函数
* Parameter		: 
*	@TxData		: 写入字节
* Return		: 读取到的字节
*	
************************************************************/
uint8_t SPI_ReadWriteByte(uint8_t TxData)
{
//	uint8_t RecevieData=0;
//	uint8_t i = 0;

//	for(i=0; i<8; i++)
//	{
//		SPI_FLASH_SCK=0;
//		if(TxData&0x80) SPI_FLASH_MOSI=1;
//		else SPI_FLASH_MOSI=0;
//		TxData<<=1;
//		SPI_FLASH_SCK=1;  // 上升沿采样
//		RecevieData<<=1;
//		if(SPI_FLASH_MISO) RecevieData |= 0x01;
//		else RecevieData &= ~0x01;   // 下降沿接收数据
//	}
//	SPI_FLASH_SCK=0;  // idle情况下SCK为电平
//	return RecevieData;
	
	u8 retry=0;				 	
	while((SPI3->SR&SPI_I2S_FLAG_TXE)==RESET) //检查指定的SPI标志位设置与否:发送缓存空标志位
	{
		retry++;
		if(retry>200)return 0;
	}			  
	SPI3->DR=TxData;	 	//发送一个byte   //通过外设SPIx发送一个数据
	retry=0;

	while((SPI3->SR&SPI_I2S_FLAG_RXNE)==RESET) //检查指定的SPI标志位设置与否:接受缓存非空标志位
	{
		retry++;
		if(retry>200)return 0;
	}	  						    
	return SPI3->DR; //返回通过SPIx最近接收的数据
	
}








