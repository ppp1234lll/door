#include "rtc.h"
#include "delay.h"
#include "time.h"

static uint8_t get_week_form_time(uint16_t year,uint8_t month,uint8_t day);

/************************************************************
*
* Function name	: Is_Leap_Year
* Description	: 润年判断
* Parameter		: 
*	@year		: 年
* Return		: 
*	
************************************************************/
static uint8_t Is_Leap_Year(u16 year)
{			  
	if(year%4==0) {  			// 必须能被4整除
		if(year%100==0) {
			if(year%400==0) { 	// 如果以00结尾,还要能被400整除 	   
				return 1;
			} else {
				return 0;  
			}
		} else {
			return 1;  
		} 
	} else {
		return 0;	
	}
}	 


uint8_t const table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5}; //月修正数据表	  
//平年的月份日期表
const uint8_t mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};
/************************************************************
*
* Function name	: RTC_Set
* Description	: rtc时钟设置
* Parameter		: 
* Return		: 
*	
************************************************************/
static uint8_t RTC_Set(u16 syear,uint8_t smon,uint8_t sday,uint8_t hour,uint8_t min,uint8_t sec)
{
	u16 t;
	u32 seccount=0;
	if(syear<1970||syear>2099)return 1;	   
	for(t=1970;t<syear;t++)	// 把所有年份的秒钟相加
	{
		if(Is_Leap_Year(t))seccount+=31622400; // 闰年的秒钟数
		else seccount+=31536000;			   // 平年的秒钟数
	}
	smon-=1;
	for(t=0;t<smon;t++)	   // 把前面月份的秒钟数相加
	{
		seccount+=(u32)mon_table[t]*86400; // 月份秒钟数相加
		if(Is_Leap_Year(syear)&&t==1)seccount+=86400; // 闰年2月份增加一天的秒钟数	   
	}
	seccount+=(u32)(sday-1)*86400; // 把前面日期的秒钟数相加 
	seccount+=(u32)hour*3600; // 小时秒钟数
    seccount+=(u32)min*60;	 // 分钟秒钟数
	seccount+=sec; // 最后的秒钟加上去

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	// 使能PWR和BKP外设时钟  
	PWR_BackupAccessCmd(ENABLE);	// 使能RTC和后备寄存器访问 
	RTC_SetCounter(seccount);	// 设置RTC计数器的值

	RTC_WaitForLastTask();	// 等待最近一次对RTC寄存器的写操作完成  	
	return 0;	    
}

/************************************************************
*
* Function name	: RTC_Get
* Description	: rtc时钟获取
* Parameter		: 
* Return		: 
*	
************************************************************/
static uint8_t RTC_Get(rtc_time_t *time)
{
	static u16 daycnt=0;
	u32 timecount=0; 
	u32 temp=0;
	u16 temp1=0;	  
	
    timecount=RTC_GetCounter();	 
 	temp=timecount/86400;   //得到天数(秒钟数对应的)
	if(daycnt!=temp)//超过一天了
	{	  
		daycnt=temp;
		temp1=1970;	//从1970年开始
		while(temp>=365)
		{				 
			if(Is_Leap_Year(temp1))//是闰年
			{
				if(temp>=366)temp-=366;//闰年的秒钟数
				else {temp1++;break;}  
			}
			else temp-=365;	  //平年 
			temp1++;  
		}   
		time->year = temp1;//得到年份
		temp1=0;
		while(temp>=28)//超过了一个月
		{
			if(Is_Leap_Year(time->year)&&temp1==1)//当年是不是闰年/2月份
			{
				if(temp>=29)temp-=29;//闰年的秒钟数
				else break; 
			}
			else 
			{
				if(temp>=mon_table[temp1])temp-=mon_table[temp1];//平年
				else break;
			}
			temp1++;  
		}
		time->month=temp1+1;	//得到月份
		time->data=temp+1;  	//得到日期 
	}
	temp=timecount%86400;     		//得到秒钟数   	   
	time->hour=temp/3600;     	//小时
	time->min=(temp%3600)/60; 	//分钟	
	time->sec=(temp%3600)%60; 	//秒钟
	time->week=get_week_form_time(time->year,time->month,time->data);
	
	return 0;
}	 

/************************************************************
*
* Function name	: RTC_Init
* Description	: rtc初始化函数
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t RTC_Init(void)
{
	uint8_t temp=0;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); // 使能PWR和BKP外设时钟   
	PWR_BackupAccessCmd(ENABLE);											 // 使能后备寄存器访问  
	
	RCC_LSICmd(ENABLE);
	while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET&&temp<250) {
		temp++;
		delay_ms(10);
	}
	if(temp>=10) {
		return 1; // 初始化时钟失败,晶振有问题	    
	}
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);		// 设置RTC时钟(RTCCLK),选择LSE作为RTC时钟    
	RCC_RTCCLKCmd(ENABLE);	 // 使能RTC时钟  
	RTC_WaitForLastTask();	 // 等待最近一次对RTC寄存器的写操作完成
	RTC_WaitForSynchro();	 // 等待RTC寄存器同步  
	RTC_WaitForLastTask();	 // 等待最近一次对RTC寄存器的写操作完成
	RTC_EnterConfigMode();	 // 允许配置	
	RTC_SetPrescaler(40000); // 设置RTC预分频的值
	RTC_WaitForLastTask();	 // 等待最近一次对RTC寄存器的写操作完成
	RTC_Set(2015,1,14,17,42,55);  //设置时间	
	RTC_ExitConfigMode(); //退出配置模式  

	
	return 0; //ok
}		 				    


/* 月份数据表 */
const uint8_t cg_table_week[12]= {0,3,3,6,1,4,6,2,5,0,3,5};
/************************************************************
*
* Function name	: get_week_form_time
* Description	: 通过年月日 获取星期
* Parameter		:
* Return		:
*
************************************************************/
static uint8_t get_week_form_time(uint16_t year,uint8_t month,uint8_t day)
{
    uint16_t temp2;
    uint8_t yearH,yearL;

    yearH=year/100;
    yearL=year%100;
    // 如果为21世纪,年份数加100
    if (yearH>19)yearL+=100;
    // 所过闰年数只算1900年之后的
    temp2=yearL+yearL/4;
    temp2=temp2%7;
    temp2=temp2+day+cg_table_week[month-1];
    if (yearL%4==0&&month<3)temp2--;

    return(temp2%7);
}


/************************************************************
*
* Function name	: RTC_set_Time
* Description	: 设置时间
* Parameter		:
* Return		:
*
************************************************************/
void RTC_set_Time(rtc_time_t rtc)
{
    RTC_Set(rtc.year,rtc.month,rtc.data,rtc.hour,rtc.min,rtc.sec);
}

/************************************************************
*
* Function name	: TimeBySecond
* Description	: 根据获取到的时间戳设置时间
* Parameter		:
* Return		:
*
************************************************************/
void TimeBySecond(u32 second)
{
    struct tm *pt,t;
    rtc_time_t time_t;
    second += 8*60*60;
    pt = localtime(&second);

    if(pt == NULL)
        return;
    t=*pt;
    t.tm_year+=1900;
    t.tm_mon++;
    time_t.year  = t.tm_year;
    time_t.month = t.tm_mon;
    time_t.data  = t.tm_mday;
    time_t.hour  = t.tm_hour;
    time_t.min   = t.tm_min;
    time_t.sec   = t.tm_sec;
    /* 设置时间 */
    RTC_set_Time(time_t);
}

/************************************************************
*
* Function name	: time_to_second_function
* Description	: 时间转时间戳 - NB
* Parameter		: 
* Return		: 
*	
************************************************************/
void time_to_second_function(uint32_t *time, uint32_t *second)
{
	struct tm pt;
	
	pt.tm_year = time[0]+100;
	pt.tm_mon  = time[1] - 1;
	pt.tm_mday = time[2];
	pt.tm_hour = time[3];
	pt.tm_min  = time[4];
	pt.tm_sec  = time[5];
	
	*second = mktime(&pt);
}

/************************************************************
*
* Function name	: RTC_Get_Time
* Description	: 获取当前时间
* Parameter		:
* Return		:
*
************************************************************/
void RTC_Get_Time(rtc_time_t *rtc)
{
    RTC_Get(rtc);
}
/************************************************************
*
* Function name	: RTC_Get_Time
* Description	: 获取当前时间
* Parameter		:
* Return		:
*
************************************************************/
void RTC_Get_Time_Test(void)
{
	static rtc_time_t rtc_test;
	while(1)
	{
		RTC_Get(&rtc_test);
		printf("data:%d-%d-%d,week:%d,time:%d:%d:%d",rtc_test.year,rtc_test.month,rtc_test.data,
																		rtc_test.week,rtc_test.hour,rtc_test.min,rtc_test.sec);
		delay_ms(1000);
	}
}





