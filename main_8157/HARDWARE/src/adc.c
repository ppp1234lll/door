/*******************************************************************************
*
* @File name	: 
* @Description	: 
* @Author		: 
*	Version	Date		Modification Description
*	1.0		
*
*******************************************************************************/
#include "adc.h"
#include "tcp_client.h"
#include "malloc.h"


#define PORT_NUM 1    // 每个通道采集的数量
#define PORT_COM 3      // 通道数
#define ADC_GET_START	(200)
#define ADC_GET_END		(1100)

uint16_t ADC_Converted1[PORT_NUM][PORT_COM]={0};           // 缓存内存1
//uint16_t ADC_Converted2[PORT_NUM][PORT_COM]={0};           // 缓存内存2

adc_value_t sg_adcdata_t 			= {0};
static uint8_t sg_adc_get_time_flag = 0;
static uint8_t sg_switch_flag       = 0;

const uint8_t c_adctable[6] = {
	ACN_POWER_ADAPTER01,
	ACN_POWER_ADAPTER02,
	ACN_POWER_ADAPTER03,
	ACN_VIN_220V,
	ACN_TOTAL_CURRENT,
	ACN_FILL_LIGHT,
};


/************************************************************
*
* Function name	: ADC_GPIO_Init
* Description	: adc初始化
* Parameter		: 
* Return		: 
*	
************************************************************/
static void ADC_GPIO_Init(void)
{
	#if (NEED_MOD_FLAG == 1)
    GPIO_InitTypeDef    GPIO_Initure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);       // 使能GPIOA时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);       // 使能GPIOA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);       // 使能GPIOA时钟
     

    GPIO_Initure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6;            //PA(03456)
    GPIO_Initure.GPIO_Mode = GPIO_Mode_AN;          //模拟
    GPIO_Initure.GPIO_PuPd = GPIO_PuPd_NOPULL;      //不上下拉
    GPIO_Init(GPIOA, &GPIO_Initure);                //初始化

    
    GPIO_Initure.GPIO_Pin  = GPIO_Pin_0|GPIO_Pin_1;     //PB(01)
    GPIO_Initure.GPIO_Mode = GPIO_Mode_AN;              //模拟
    GPIO_Initure.GPIO_PuPd = GPIO_PuPd_NOPULL;          //不上下拉
    GPIO_Init(GPIOB, &GPIO_Initure);                    //初始化
    
    
    GPIO_Initure.GPIO_Pin  =GPIO_Pin_0|GPIO_Pin_2|GPIO_Pin_3;            //PC(023)
    GPIO_Initure.GPIO_Mode =GPIO_Mode_AN;           //模拟
    GPIO_Initure.GPIO_PuPd =GPIO_PuPd_NOPULL;       //不上下拉
	
    GPIO_Init(GPIOC, &GPIO_Initure);                //初始化
	
	#endif
}

/*
HCLK = 168M
PCLK2 = 84M
PCLK1 = 42M
T = 84/分频数/((采样间隔+12个采样间隔)xN(通道数)) // 一轮adc的时间，每个通道一个数据

*/

/************************************************************
*
* Function name	: Rheostat_ADC_Mode_Config
* Description	: ADCDMA模式
* Parameter		: 
* Return		: 
*	
************************************************************/
void Rheostat_ADC_Mode_Config(void)
{
	#if (NEED_MOD_FLAG == 1)
	NVIC_InitTypeDef NVIC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_GPIO_Init();
    // 开启ADC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 , ENABLE);
    // 开启DMA时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

    // ------------------DMA Init 结构体参数初始化------------------------

    // 选择 DMA 通道，通道存在于数据流中
    DMA_InitStructure.DMA_Channel = DMA_Channel_0;
    // 外设基址为：ADC 数据寄存器地址
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&ADC1->DR;
    // 存储器地址
    DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)ADC_Converted1;
    // 数据传输方向为外设到存储器
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
    // 缓冲区大小，指一次传输的数据项
    DMA_InitStructure.DMA_BufferSize = PORT_NUM*PORT_COM;
    // 外设寄存器只有一个，地址不用递增
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    // 存储器地址递增
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    //DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;
    // // 外设数据大小为半字，即两个字节
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    // 存储器数据大小也为半字，跟外设数据大小相同
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    // 循环传输模式
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    // DMA 传输通道优先级为高，当使用一个DMA通道时，优先级设置不影响
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    // 禁止DMA FIFO ，使用直连模式
    DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
    // FIFO 阈值大小，FIFO模式禁止时，这个不用配置
    DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
    // 存储器突发单次传输
    DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
    // 外设突发单次传输
    DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	
//	DMA_DoubleBufferModeConfig(DMA2_Stream0,(uint32_t)ADC_Converted1,DMA_Memory_0);
//	DMA_DoubleBufferModeConfig(DMA2_Stream0,(uint32_t)ADC_Converted2,DMA_Memory_1);
//	DMA_DoubleBufferModeCmd(DMA2_Stream0,ENABLE);
	
    //初始化DMA数据流，流相当于一个大的管道，管道里面有很多通道
    DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
	 
    // 使能DMA数据流
	NVIC_InitStructure.NVIC_IRQChannel=DMA2_Stream0_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);

    DMA_Cmd(DMA2_Stream0, ENABLE);
    // -------------------ADC Common 结构体参数初始化--------------------
    // 独立ADC模式
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    // 时钟为fpclk x分频
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8;
    // 禁止DMA直接访问模式
    ADC_CommonInitStructure.ADC_DMAAccessMode=ADC_DMAAccessMode_Disabled;
    // 采样时间间隔
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);
    // -------------------ADC Init 结构体参数初始化---------------------
    // ADC 分辨率
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    // 开启扫描模式
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;
    // 连续转换
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
    //禁止外部边沿触发
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    //使用软件触发，外部触发不用配置，注释掉即可
    //ADC_InitStructure.ADC_ExternalTrigConv=ADC_ExternalTrigConv_T1_CC1;
    //数据右对齐
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    //转换通道 x个
    ADC_InitStructure.ADC_NbrOfConversion = PORT_COM;
    ADC_Init(ADC1, &ADC_InitStructure);
    //------------------------------------------------------------------
    // 配置 ADC 通道转换顺序和采样时间周期
//    ADC_RegularChannelConfig(ADC1, ADC_Channel_0,  1,  ADC_SampleTime_112Cycles );  //ADC1,ADC通道,480个周期,提高采样时间可以提高精确度 
//    ADC_RegularChannelConfig(ADC1, ADC_Channel_3,  2,  ADC_SampleTime_112Cycles );
//    ADC_RegularChannelConfig(ADC1, ADC_Channel_4,  3,  ADC_SampleTime_112Cycles );
//    ADC_RegularChannelConfig(ADC1, ADC_Channel_5,  4,  ADC_SampleTime_112Cycles );
//    ADC_RegularChannelConfig(ADC1, ADC_Channel_6,  5,  ADC_SampleTime_112Cycles );
//    ADC_RegularChannelConfig(ADC1, ADC_Channel_8,  6,  ADC_SampleTime_112Cycles );
    ADC_RegularChannelConfig(ADC1, ADC_Channel_10,  1, ADC_SampleTime_480Cycles );
    ADC_RegularChannelConfig(ADC1, ADC_Channel_12,  2, ADC_SampleTime_480Cycles );
    ADC_RegularChannelConfig(ADC1, ADC_Channel_13,  3, ADC_SampleTime_480Cycles );
//	ADC_RegularChannelConfig(ADC1, ADC_Channel_9,   4, ADC_SampleTime_480Cycles );
    // 使能DMA请求 after last transfer (Single-ADC mode)
    ADC_DMARequestAfterLastTransferCmd(ADC1, ENABLE);
    // 使能ADC DMA
    ADC_DMACmd(ADC1, ENABLE);
    // 使能ADC
    ADC_Cmd(ADC1, ENABLE);
    //开始adc转换，软件触发
    ADC_SoftwareStartConv(ADC1);
	
	/* 缓存区初始化 */
	memset(g_adcqueue_t,0,sizeof(g_adcqueue_t));
	myqueue_queue_init(&g_adcqueue_t[0]);
//	myqueue_queue_init(&g_adcqueue_t[1]);
//	myqueue_queue_init(&g_adcqueue_t[2]);
	#endif
}


void DMA2_Stream0_IRQHandler(void) 
{
	#if (NEED_MOD_FLAG == 1)
	 if(DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
	 {
		 DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
		 DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, DISABLE);
		 /* 数据拷贝 */
//		 DMA_Cmd(DMA2_Stream0, DISABLE);
		 sg_adc_get_time_flag = 1;
//		 if(DMA_GetCurrentMemoryTarget(DMA2_Stream0) == 1) 
//		 {
//			 sg_switch_flag = 0;
//		 }
//		 else
//		 {
//			 sg_switch_flag = 1;
//			 
//		 }
	 }
	 ADC_Cmd(ADC1, DISABLE);
	 DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
	#endif
}

//void adc_cacke_data_timer(void)
//{
//	uint16_t i = 0;
//	uint16_t j = 0;
//	
//	if(sg_adc_get_time_flag == 1) {
//		sg_adc_get_time_flag = 0;
//		for(j=0; j<PORT_COM; j++) {
//			for(i=0; i<PORT_NUM; i++) {
//				if(sg_switch_flag == 1) {
//					myqueue_storage_cache_one_data(&g_adcqueue_t[j],ADC_Converted2[i][j]);
//				} else {
//					myqueue_storage_cache_one_data(&g_adcqueue_t[j],ADC_Converted1[i][j]);
//				}	
//			}
//		}
//	}
//}

/************************************************************
*
* Function name	: 
* Description	: 
* Parameter		: 
* Return		: 
*	
************************************************************/
u16 filter(u16 ch[PORT_NUM][PORT_COM],uint16_t cha,uint16_t GetM,uint16_t Num)
{
	u16 count,i,j;
	u16 temp;
	u32 sum=0;

	for (j=0;j<GetM-1;j++)
	{
		for (i=j+1;i<GetM;i++)
		{
			if ( ch[j][cha]>ch[i][cha])
			{
				temp = ch[j][cha];
				ch[j][cha] = ch[i][cha]; 
				ch[i][cha] = temp;
			}
		}
	}
	for(count=Num/2;count<GetM-Num/2;count++) sum += ch[count][cha];
	return (u16)(sum/(GetM-Num));
}



/************************************************************
*
* Function name	: adc_get_timer_function
* Description	: adc对应的时间任务
* Parameter		: 
* Return		: 
*	
************************************************************/
void adc_get_timer_function(void)
{
//	static uint8_t count = 0;
//	
//	/* 保持1ms一次 - 50hz */
//	count++;
//	if(count >= 20)
//	{
//		count = 0;
//		sg_adc_get_time_flag = 1;
//	}
}

/************************************************************
*
* Function name	: adc_get_max_data
* Description	: 获取最大值
* Parameter		: 
* Return		: 
*	
************************************************************/
uint32_t adc_get_max_data(uint8_t num)
{
	uint16_t  	   max   = 0;
//	uint16_t	   index = 0;
//	
//	for(index=0; index<PORT_NUM; index++ )
//	{
//		if(sg_switch_flag == 1)
//		{
//			if(max < ADC_Converted2[index][num-1])
//			{
//				max = ADC_Converted2[index][num-1];
//			}
//		}
//		else
//		{
//			if(max < ADC_Converted1[index][num-1])
//			{
//				max = ADC_Converted1[index][num-1];
//			}
//		}	
//	}
	
	return max;
}

uint16_t adc_get_max_data_function(uint16_t *buff,uint16_t size)
{
	uint16_t i = 0;
	uint16_t max = 0;
	
	for(i=0; i<size; i++) {
		if(max < buff[i]) {
			max = buff[i];
		}
	}
	return max;
}

/************************************************************
*
* Function name	: adc_deal_data_function
* Description	: adc数据处理
* Parameter		: 
* Return		: 
*	
************************************************************/
void adc_deal_data_function(void)
{
	uint16_t 	   *buff = 0;
	fp32		   ftemp = 0;
	uint16_t       temp  = 0;
	uint16_t	   index = 0;
	uint16_t       i	 = 0;
	uint8_t		   j 	 = 0;
	
	/* 是否需要处理ADC数据 */
	if(sg_adc_get_time_flag != 1)
	{
		return ;
	}
	sg_adc_get_time_flag = 0;
	
	buff = mymalloc(SRAMIN,sizeof(ADC_Converted1));
	if(buff == NULL) {
		ADC_Cmd(ADC1, ENABLE);
		//开始adc转换，软件触发
		#if (NEED_MOD_FLAG == 1)
		ADC_SoftwareStartConv(ADC1);
		#endif
		return ;
	}
	
	/* 数据填充 */
	index = 0;
	for(j=0; j<PORT_COM; j++) {
		for(i=0; i<PORT_NUM; i++) {
			buff[index++] = ADC_Converted1[i][j];
		}
	}
	/* 数据计算 */
	/* 220V总电压 */
	temp = adc_get_max_data_function(&buff[0],PORT_NUM);
	ftemp = temp;
	sg_adcdata_t.adc_vin_220v 		= ftemp*1000*3.3/4096;
	
	#if (NEED_MOD_FLAG == 1)
	/* 220V总电流 */
	temp = average_test_function(&buff[PORT_NUM],PORT_NUM);
	ftemp = temp;
	sg_adcdata_t.adc_total_current = ftemp*1000*3.3/4096;
	
	/* 补光灯电流 */
	temp = average_test_function(&buff[PORT_NUM*2],PORT_NUM);
	ftemp = temp;
	sg_adcdata_t.adc_fill_light = ftemp*1000*3.3/4096;
	
	ADC_Cmd(ADC1, ENABLE);
    //开始adc转换，软件触发
    ADC_SoftwareStartConv(ADC1);
#endif
	myfree(SRAMIN,buff);
}


/************************************************************
*
* Function name	: adc_get_collection_data_function
* Description	: 获取adc采集的数据
* Parameter		: 
* Return		: 
*	
************************************************************/
adc_value_t adc_get_collection_data_function(void)
{
	return sg_adcdata_t;
}

