#include "lis3dh_spi.h"
#include "delay.h"

/*
	8、3轴加速度计LIS3DH: (模拟IIC方式)，引脚分配为：
		SCL:   PE5
		SDA:   PE4  MOSI
		SDO:   PE3  MISO
		CS:    PE2
*/
/* 参数 */

#define LIS3DH_SPI_SCLK_GPIO_CLK				    RCC_APB2Periph_GPIOE
#define LIS3DH_SPI_SCLK_GPIO_PORT           GPIOE
#define LIS3DH_SPI_SCLK_PIN                 GPIO_Pin_5

#define LIS3DH_SPI_MOSI_GPIO_CLK				    RCC_APB2Periph_GPIOE
#define LIS3DH_SPI_MOSI_GPIO_PORT           GPIOE
#define LIS3DH_SPI_MOSI_PIN                 GPIO_Pin_4

#define LIS3DH_SPI_MISO_GPIO_CLK				    RCC_APB2Periph_GPIOE
#define LIS3DH_SPI_MISO_GPIO_PORT           GPIOE
#define LIS3DH_SPI_MISO_PIN                 GPIO_Pin_3

#define LIS3DH_SPI_SCK	PEout(5)  	// SCL
#define LIS3DH_SPI_MOSI	PEout(4) 	  // SDA
#define LIS3DH_SPI_MISO	PEin(3)   	// SDO
 	    		
/************************************************************
*
* Function name	: LIS3DH_SPI_INIT
* Description	: 初始化SPI
* Parameter		: 
* Return		: 
*	
************************************************************/
void LIS3DH_SPI_INIT(void)
{	
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(LIS3DH_SPI_SCLK_GPIO_CLK|LIS3DH_SPI_MOSI_GPIO_CLK|
												 LIS3DH_SPI_MISO_GPIO_CLK,ENABLE);

	GPIO_InitStructure.GPIO_Pin   = LIS3DH_SPI_SCLK_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LIS3DH_SPI_SCLK_GPIO_PORT,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin   = LIS3DH_SPI_MOSI_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LIS3DH_SPI_MOSI_GPIO_PORT,&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin   = LIS3DH_SPI_MISO_PIN;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LIS3DH_SPI_MISO_GPIO_PORT,&GPIO_InitStructure);
}  

/************************************************************
*
* Function name	: SPI2_ReadWriteByte
* Description	: 读写一个字节
* Parameter		: 
*	@TxData		: 写入字节
* Return		: 字节
*	
************************************************************/
uint8_t LIS3DH_SPI_ReadWriteByte(uint8_t TxData)
{		
	uint8_t RecevieData=0;
	uint8_t i;

	for(i=0;i<8;i++)  
	{  
		LIS3DH_SPI_SCK=0;  
		if(TxData&0x80) 
			LIS3DH_SPI_MOSI=1; 
		else 
			LIS3DH_SPI_MOSI=0;
		TxData<<=1;  
		LIS3DH_SPI_SCK=1;  //上升沿采样  
		RecevieData<<=1;  
		if(LIS3DH_SPI_MISO) 
			RecevieData |= 0x01;
		else 
			RecevieData &= ~0x01;   //下降沿接收数据  
	}  
	LIS3DH_SPI_SCK=0;  //idle情况下SCK为电平  
	return RecevieData;				    
}








